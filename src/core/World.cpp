#include "core/World.h"

#include <cmath>

namespace drone {

namespace {

// Por debajo de esta velocidad de impacto un contacto es "posarse", no
// "chocar": sin el umbral, apoyarse en el suelo publicaría una colisión de
// 0 m/s en cada aterrizaje y el HUD avisaría de choques inexistentes.
constexpr float kMinReportedImpact = 0.5f;

// Holgura para dar el dron por posado. El solver deja una penetración
// residual de fracciones de milímetro, así que una comparación exacta
// contra 0 parpadearía.
constexpr float kGroundTolerance = 0.05f;

// Ayuda de enderezado. El casco es una esfera y al aterrizar rueda a
// cualquier postura; volcado, el empuje apunta al suelo y el dron no puede
// despegar ni siquiera con el mando a fondo. Por encima de esta inclinacion
// se corta el empuje y un par PD lo devuelve a la horizontal.
// Entra en recuperacion por debajo de este "arriba" y no la suelta hasta
// estar bien nivelado y quieto: sin histeresis el empuje volvia justo en el
// limite, lo volcaba otra vez y el dron se quedaba dando tumbos en el sitio.
constexpr float kRightingEnter = 0.85f;    // ~32 grados de inclinacion
constexpr float kRightingExit = 0.97f;     // ~14 grados
constexpr float kRightingCalmSpin = 1.0f;  // rad/s
constexpr float kRightingTorque = 1.0f;    // N*m con el dron del reves
constexpr float kRightingDamping = 0.15f;  // N*m por rad/s, para no rebotar

constexpr float kGroundHalfThickness = 0.5f;

// Segundos entre dos premios consecutivos por la misma zona de aterrizaje.
constexpr float kLandingZoneCooldown = 2.0f;

float clamp01(float v) {
    return v < 0.0f ? 0.0f : (v > 1.0f ? 1.0f : v);
}

}  // namespace

World::World(const GameConfig& gameCfg, const physics::PhysicsSettings& physCfg)
    : m_gameCfg(gameCfg),
      m_physics(physCfg),
      m_drone(gameCfg),
      m_environment(gameCfg),
      m_lastBattery(gameCfg.batteryMax) {
    // La cara superior del suelo se sitúa en y = -droneRadius, de modo que
    // la esfera del dron reposa con su CENTRO en y = 0. Así "altitud 0"
    // sigue significando "posado" para el HUD, los guardados y los tests:
    // la migración a rp3d no cambia lo que el jugador ve.
    const float groundTop = -m_gameCfg.droneRadius;
    m_groundBody = m_physics.createBoxBody(
        {m_gameCfg.worldHalfExtent, kGroundHalfThickness, m_gameCfg.worldHalfExtent},
        {0.0f, groundTop - kGroundHalfThickness, 0.0f}, physics::BodyType::Static);

    m_droneBody = m_physics.createSphereBody(m_gameCfg.droneRadius, spawnPosition(),
                                             physics::BodyType::Dynamic, m_gameCfg.droneMass);
    // Materiales
    m_physics.setFriction(m_droneBody, m_gameCfg.droneFriction);
    m_physics.setBounciness(m_droneBody, m_gameCfg.droneBounciness);
    m_physics.setLinearDamping(m_droneBody, m_gameCfg.linearDamping);
    m_physics.setAngularDamping(m_droneBody, m_gameCfg.angularDamping);
    m_physics.setFriction(m_groundBody, m_gameCfg.groundFriction);
    m_physics.setBounciness(m_groundBody, m_gameCfg.groundBounciness);

    // Sin corregir la inercia de la esfera, el PID no consigue mover la
    // actitud a tiempo y el dron parece que no responde.
    const float inertia = m_gameCfg.droneInertia;
    m_physics.setInertiaTensor(m_droneBody, {inertia, inertia, inertia});

    // El empuje de hover se DERIVA de la masa y la gravedad reales. Fijarlo
    // a una constante hacía que cambiar la masa en el TOML descuadrase el
    // hover y el failsafe sin que nada lo avisara.
    m_gravity = std::fabs(physCfg.gravity.y);
    if (m_gameCfg.maxThrust > 0.0f)
        m_hoverThrottle = clamp01(m_gameCfg.droneMass * m_gravity / m_gameCfg.maxThrust);
    m_altHold.hoverThrust = m_hoverThrottle;

    applyControlConfig();
    // Las plataformas ya no se crean aqui: las pone el entorno, asi que
    // aparecen al cargarlo. Un World sin entorno cargado no tiene ninguna.
}

void World::applyControlConfig() {
    m_mixer.armLength = m_gameCfg.armLength;
    m_mixer.yawTorqueFactor = m_gameCfg.yawTorqueFactor;

    // Las ganancias llegan en las unidades del original (grados, cuentas
    // PWM) y se convierten aquí de una vez. Copiarlas crudas las hacía ~35
    // veces más agresivas de lo que eran en el dron real.
    const float k = DronePID::kFromCortex;
    m_pid.rollKp = m_gameCfg.pidRollKp * k;
    m_pid.rollKi = m_gameCfg.pidRollKi * k;
    m_pid.rollKd = m_gameCfg.pidRollKd * k;
    m_pid.pitchKp = m_gameCfg.pidPitchKp * k;
    m_pid.pitchKi = m_gameCfg.pidPitchKi * k;
    m_pid.pitchKd = m_gameCfg.pidPitchKd * k;
    m_pid.yawKp = m_gameCfg.pidYawKp * k;
    m_pid.yawKi = m_gameCfg.pidYawKi * k;
    m_pid.yawFF = m_gameCfg.pidYawFF * k;

    m_altHold.kp = m_gameCfg.altitudeHoldKp;
    m_yawHold.holdKp = m_gameCfg.yawHoldKp;
    m_failsafe.timeoutSeconds = m_gameCfg.failsafeTimeout;
    m_failsafe.descentPerSecond = m_gameCfg.failsafeDescentPerSecond;
}

Vec3 World::spawnPosition() const {
    return {0.0f, 0.0f, 0.0f};
}

void World::loadEnvironment(const std::string& name) {
    m_environment.loadEnvironment(name);
    createObstacles();
    if (m_gameCfg.landingZonesEnabled)
        createLandingZones();
}

void World::createObstacles() {
    // Recargar un entorno sustituye los obstáculos: sin esto los cuerpos del
    // entorno anterior seguirían colisionando, invisibles para el jugador.
    for (const physics::BodyId id : m_obstacleBodies)
        m_physics.destroyBody(id);
    m_obstacleBodies.clear();

    for (const Obstacle& obs : m_environment.obstacles()) {
        auto id = m_physics.createBoxBody(obs.size * 0.5f, obs.center, physics::BodyType::Static);
        m_physics.setFriction(id, m_gameCfg.obstacleFriction);
        m_physics.setBounciness(id, m_gameCfg.obstacleBounciness);
        m_obstacleBodies.push_back(id);
    }
}

void World::createLandingZones() {
    // Las posiciones las decide el entorno, que es quien sabe donde ha
    // dejado hueco: tenerlas aqui duplicadas obligaba a que ambos ficheros
    // dijeran lo mismo, y el generador podia plantar una torre encima.
    for (const physics::BodyId id : m_landingZoneBodies)
        m_physics.destroyBody(id);
    m_landingZoneBodies.clear();

    const float r = m_gameCfg.landingZoneRadius;
    const float h = m_gameCfg.landingZoneHeight;
    for (const Vec3& pos : m_environment.landingZones()) {
        auto id = m_physics.createBoxBody({r, h, r}, pos, physics::BodyType::Static);
        m_physics.setTrigger(id, true);
        m_landingZoneBodies.push_back(id);
    }
}

void World::checkLandingZones() {
    // El cooldown es de la instancia, no `static` local: una variable
    // estática la comparten todos los World del proceso, así que reiniciar
    // la partida —o el test siguiente— heredaba el instante del anterior.
    for (const physics::BodyId zoneId : m_landingZoneBodies) {
        if (!m_physics.triggerOverlap(m_droneBody, zoneId))
            continue;
        if (m_simTime - m_lastLandingXpTime > kLandingZoneCooldown) {
            m_bus.publish({EventType::LandingZone, static_cast<float>(m_gameCfg.landingZoneXP)});
            m_lastLandingXpTime = m_simTime;
        }
        break;
    }
}

void World::teleportDrone(const Vec3& position, const Vec3& velocity) {
    m_physics.resetBody(m_droneBody, position);
    m_physics.setLinearVelocity(m_droneBody, velocity);
    m_drone.setPosition(position);
    m_drone.setVelocity(velocity);
}

void World::setDroneOrientation(float qx, float qy, float qz, float qw) {
    m_physics.setOrientation(m_droneBody, qx, qy, qz, qw);
}
void World::syncDroneToPhysics() {
    const float dt = m_gameCfg.fixedTimestep;
    const Vec3 input = m_drone.thrustInput();

    updateRightingState();

    // Volcado en el suelo no se giran las hélices: su empuje apunta contra el
    // suelo y, con el PID peleando contra una actitud imposible, el par que
    // genera vencía a la ayuda de enderezado y el dron no se recuperaba nunca.
    // Solo se endereza; el arrastre se sigue aplicando más abajo.
    if (m_recovering) {
        for (float& t : m_motorThrust)
            t = 0.0f;
        m_yawTorque = 0.0f;
        m_pid.reset();
        applyRightingAssist();
        const Vec3 stopped = m_physics.getLinearVelocity(m_droneBody);
        m_physics.applyForce(m_droneBody, stopped * -m_gameCfg.dragCoefficient);
        return;
    }

    // Un quad se traslada INCLINÁNDOSE: los mandos horizontales piden
    // actitud, no fuerza. Antes solo se leía input.y y los cuatro mandos
    // horizontales no llegaban a la física — el dron no podía avanzar.
    //   avanzar (+Z) ⇒ morro abajo    ⇒ pitch negativo
    //   derecha (+X) ⇒ lado derecho abajo ⇒ roll negativo
    const float deg2rad = 0.0174533f;
    const float maxTilt = m_gameCfg.maxTiltDeg * deg2rad;
    const float targetRoll = -input.x * maxTilt + m_trim.rollDeg * deg2rad;
    const float targetPitch = -input.z * maxTilt + m_trim.pitchDeg * deg2rad;

    float throttle = std::max(0.0f, input.y);

    // Asistencia de vuelo: inclinar sin gas no produce movimiento alguno,
    // así que pedir desplazamiento horizontal garantiza al menos el empuje
    // que sostiene el peso. Sin esto, WASD solo funcionaría manteniendo Q.
    const float horizontal = std::max(std::fabs(input.x), std::fabs(input.z));
    if (horizontal > 0.0f && input.y >= 0.0f)
        throttle = std::max(throttle, m_hoverThrottle);

    if (!m_drone.hasBattery())
        throttle = 0.0f;

    if (m_altHold.engaged) {
        throttle = clamp01(m_hoverThrottle + m_altHold.compute(m_drone.position().y));
    } else {
        throttle = m_failsafe.compute(throttle, m_hoverThrottle, m_drone.isGrounded(), dt);
    }

    // PID sobre la actitud REAL. Antes recibía ceros constantes, así que
    // creía que el dron estaba siempre nivelado y no estabilizaba nada.
    const Vec3 angVel = m_physics.getAngularVelocity(m_droneBody);
    // De velocidad angular del cuerpo a tasa de cada ángulo. Con esta
    // convención el alabeo es un giro POSITIVO sobre Z, pero el cabeceo
    // (morro arriba) es un giro NEGATIVO sobre X: pasar la componente cruda
    // invertía el signo de la amortiguación de cabeceo y el dron volcaba.
    const float rollRate = angVel.z;
    const float pitchRate = -angVel.x;
    const float yawRate = angVel.y;

    const float yawRateCmd = m_yawHold.compute(0.0f, m_drone.yaw());

    float rollCorr = 0, pitchCorr = 0, yawCorr = 0;
    m_pid.compute(targetRoll, targetPitch, yawRateCmd, m_drone.roll(), m_drone.pitch(), rollRate,
                  pitchRate, yawRate, dt, throttle > 0.01f, rollCorr, pitchCorr, yawCorr);

    const XFrameMixer::Result mix =
        m_mixer.compute(throttle, rollCorr, pitchCorr, yawCorr, m_gameCfg.maxThrust);

    // Retardo de primer orden del conjunto motor+ESC: un escalón de empuje
    // no es instantáneo.
    const float tau = std::max(1e-4f, m_gameCfg.motorTimeConstant);
    const float lag = std::min(1.0f, dt / tau);

    for (int i = 0; i < 4; ++i) {
        m_motorThrust[i] += (mix.motors[i].thrust - m_motorThrust[i]) * lag;
        if (m_motorThrust[i] <= 0.0f)
            continue;
        // Fuerza y punto EN EL MARCO DEL CUERPO: el empuje sigue la
        // inclinación del chasis (que es lo que produce el desplazamiento
        // horizontal) y el punto de aplicación acompaña al dron. Con la
        // variante de mundo, el par dependía de dónde estuviera en el mapa.
        m_physics.applyLocalForceAtLocalPosition(m_droneBody, {0.0f, m_motorThrust[i], 0.0f},
                                                 mix.motors[i].position);
    }

    // El par de reacción es lineal con el empuje: arrastra el mismo retardo.
    m_yawTorque += (mix.yawTorque - m_yawTorque) * lag;
    m_physics.applyLocalTorque(m_droneBody, {0.0f, m_yawTorque, 0.0f});

    // El arrastre se aplica SIEMPRE, también sin batería: antes el retorno
    // temprano se lo saltaba y un dron sin batería caía en el vacío.
    //
    // Pero el viento solo empuja en vuelo. Posado, el dron es una esfera que
    // rueda sin resistencia, así que una brisa de 3 m/s lo arrastraba por el
    // mapa para siempre: con la batería agotada nunca bajaba de 0,1 m/s y el
    // fin de partida no llegaba jamás — la partida se quedaba colgada.
    // Sigue habiendo resistencia del aire, que es lo que lo frena hasta parar.
    const Vec3 airVelocity = m_drone.isGrounded() ? Vec3{} : m_environment.wind();
    const Vec3 vel = m_physics.getLinearVelocity(m_droneBody);
    m_physics.applyForce(m_droneBody, (airVelocity - vel) * m_gameCfg.dragCoefficient);

    applyRightingAssist();
}

// Un quad volcado no puede enderezarse con sus propias hélices: el empuje
// apunta al suelo y la partida se queda muerta sin llegar a "fin de partida",
// porque el jugador pulsa para subir y no ocurre nada. El casco es una esfera
// y rueda a cualquier postura al aterrizar, asi que hace falta una ayuda:
// posado y tumbado, un par lo devuelve a la horizontal.
// En el aire no hay nada que hacer: un quad no puede darse la vuelta con sus
// propias helices, asi que caera y se recuperara ya en el suelo.
void World::updateRightingState() {
    if (!m_drone.isGrounded()) {
        m_recovering = false;
        return;
    }
    const float up = bodyUp().y;
    if (up <= kRightingEnter) {
        m_recovering = true;
        return;
    }
    if (!m_recovering)
        return;
    const Vec3 spin = m_physics.getAngularVelocity(m_droneBody);
    const bool quieto = std::fabs(spin.x) + std::fabs(spin.z) < kRightingCalmSpin;
    if (up >= kRightingExit && quieto)
        m_recovering = false;
}

void World::applyRightingAssist() {
    const Vec3 up = bodyUp();

    // Eje que lleva el "arriba" del dron hacia la vertical del mundo: el
    // producto vectorial up × (0,1,0).
    Vec3 axis{-up.z, 0.0f, up.x};
    const float len = axis.length();
    // Exactamente invertido: el eje degenera y vale cualquiera perpendicular.
    axis = (len < 1e-3f) ? Vec3{1.0f, 0.0f, 0.0f} : axis * (1.0f / len);

    // Máximo con el dron del revés y suave al acercarse al umbral, para que
    // la ayuda no dé tirones cuando solo está algo inclinado.
    const float strength = (1.0f - up.y) * 0.5f;

    // Sin término derivativo el par constante lo pasaba de largo y el dron se
    // quedaba balanceándose sobre el suelo. Se amortigua el giro en los ejes
    // horizontales; el de guiñada (Y) se deja libre, que es del jugador.
    const Vec3 spin = m_physics.getAngularVelocity(m_droneBody);
    const Vec3 damping{-spin.x * kRightingDamping, 0.0f, -spin.z * kRightingDamping};

    m_physics.applyTorque(m_droneBody, axis * (kRightingTorque * strength) + damping);
}

Vec3 World::bodyUp() const {
    const physics::Transform t = m_physics.getTransform(m_droneBody);
    const float x = t.qx, y = t.qy, z = t.qz, w = t.qw;
    return {2.0f * (x * y - z * w), 1.0f - 2.0f * (x * x + z * z), 2.0f * (y * z + x * w)};
}

void World::updateAttitude() {
    const physics::Transform t = m_physics.getTransform(m_droneBody);
    const float x = t.qx, y = t.qy, z = t.qz, w = t.qw;

    // Ejes del cuerpo en coordenadas de mundo, sacados del cuaternión.
    const Vec3 right{1.0f - 2.0f * (y * y + z * z), 2.0f * (x * y + z * w), 2.0f * (x * z - y * w)};
    const Vec3 forward{2.0f * (x * z + y * w), 2.0f * (y * z - x * w),
                       1.0f - 2.0f * (x * x + y * y)};

    // El "arriba" del dron visto desde el mundo: con up.y < 0 está volcado.
    const Vec3 up = bodyUp();

    // Con asin(right.y) el ángulo salía siempre dentro de ±90°, así que un
    // dron boca abajo (165° de alabeo) se informaba como 15° y ni el HUD ni
    // el PID se enteraban: el empuje apuntaba al suelo y ya no despegaba.
    // atan2 contra la componente vertical del "arriba" cubre los 360°.
    // roll > 0 ⇒ lado derecho arriba; pitch > 0 ⇒ morro arriba.
    m_drone.setAttitude(std::atan2(right.y, up.y), std::atan2(forward.y, up.y),
                        std::atan2(forward.x, forward.z));
}

void World::syncDroneFromPhysics() {
    Vec3 position = m_physics.getTransform(m_droneBody).position;
    // El contacto de reposo deja una penetración de fracciones de milímetro.
    // Es inherente a un solver de impulsos, pero reportarla hacía que el HUD
    // mostrase "Altitud: -0.0 m" con el dron simplemente posado.
    if (position.y < 0.0f && position.y > -kGroundTolerance)
        position.y = 0.0f;

    m_drone.setPosition(position);
    m_drone.setVelocity(m_physics.getLinearVelocity(m_droneBody));
    updateAttitude();

    publishContacts();
    applyWorldBounds();

    // El consumo se cobra por el empuje que REALMENTE dan los motores, no
    // por lo que pide el mando. Con el mando como medida, el altitude hold
    // volaba indefinidamente con la batería al 100 %: sostiene el dron sin
    // que el jugador toque nada, así que la entrada era cero.
    float totalThrust = 0.0f;
    for (const float t : m_motorThrust)
        totalThrust += t;
    m_drone.drainBattery(totalThrust * m_gameCfg.batteryPerNewton * m_gameCfg.fixedTimestep);
    publishBatteryEvents();
}

void World::publishContacts() {
    bool grounded = m_drone.position().y <= kGroundTolerance;

    for (const physics::ContactEvent& c : m_physics.contacts()) {
        if ((c.a != m_droneBody && c.b != m_droneBody) || c.phase == physics::ContactPhase::Exit)
            continue;

        if (c.a == m_groundBody || c.b == m_groundBody)
            grounded = true;

        // Solo el Enter: mientras el dron roza una pared, ContactStay llega
        // cada frame y publicarlos todos convertiría un roce en una ráfaga
        // de avisos idénticos.
        if (c.phase == physics::ContactPhase::Enter && c.impactSpeed >= kMinReportedImpact)
            m_bus.publish({EventType::Collision, c.impactSpeed});
    }

    m_drone.setGrounded(grounded);
}

void World::applyWorldBounds() {
    Vec3 p = m_drone.position();
    Vec3 v = m_drone.velocity();
    bool clamped = false;

    const auto clampAxis = [&](float& pos, float& vel, float limit) {
        if (std::fabs(pos) <= limit)
            return;
        pos = std::copysign(limit, pos);
        if (vel * pos > 0.0f)
            vel = 0.0f;
        clamped = true;
    };

    clampAxis(p.x, v.x, m_gameCfg.worldHalfExtent);
    clampAxis(p.z, v.z, m_gameCfg.worldHalfExtent);
    if (p.y > m_gameCfg.maxAltitude) {
        p.y = m_gameCfg.maxAltitude;
        if (v.y > 0.0f)
            v.y = 0.0f;
        clamped = true;
    }

    if (!clamped)
        return;

    // El techo y las paredes del mundo no son cuerpos: se imponen moviendo
    // el cuerpo en rp3d. Tocar solo el estado de juego no serviría de nada,
    // porque el siguiente paso lo sobrescribe desde el motor.
    m_physics.setPosition(m_droneBody, p);
    m_physics.setLinearVelocity(m_droneBody, v);
    m_drone.setPosition(p);
    m_drone.setVelocity(v);
}

void World::publishBatteryEvents() {
    const float battery = m_drone.battery();
    if (m_lastBattery > 0.0f && battery <= 0.0f)
        m_bus.publish({EventType::BatteryEmpty, 0.0f});
    else if (m_lastBattery > m_gameCfg.batteryLowThreshold &&
             battery <= m_gameCfg.batteryLowThreshold && battery > 0.0f)
        m_bus.publish({EventType::BatteryLow, battery});
    m_lastBattery = battery;
}

void World::step(float dt) {
    m_environment.step(dt);
    syncDroneToPhysics();
    m_physics.step(dt);
    // publishContacts() ya lo llama syncDroneFromPhysics(), que necesita el
    // estado de "posado" antes de aplicar los límites del mundo. Llamarlo
    // otra vez aquí publicaba cada choque por duplicado.
    syncDroneFromPhysics();

    // Landing zone XP
    if (m_drone.isGrounded() && m_drone.velocity().length() < 0.1f) {
        checkLandingZones();
    }

    m_simTime += dt;
}

void World::restoreSimTime(float simTime) {
    m_simTime = simTime < 0.0f ? 0.0f : simTime;
    m_environment.restoreProgress(m_simTime);
}

void World::reset() {
    m_drone.reset();
    m_environment.reset();
    m_simTime = 0.0f;
    m_lastBattery = m_gameCfg.batteryMax;
    // Sin esto el dron reaparece donde se estrelló y con su velocidad: el
    // estado de juego se reinicia, pero el cuerpo de rp3d no.
    teleportDrone(spawnPosition());
}

WorldState World::snapshot() const {
    WorldState s;
    s.dronePosition = m_drone.position();
    s.droneVelocity = m_drone.velocity();
    s.wind = m_environment.wind();
    s.battery = m_drone.battery();
    s.difficulty = m_environment.difficulty();
    s.simTime = m_simTime;
    s.environmentName = m_environment.name();
    s.obstacles = m_environment.obstacles();
    auto t = m_physics.getTransform(m_droneBody);
    s.droneQx = t.qx;
    s.droneQy = t.qy;
    s.droneQz = t.qz;
    s.droneQw = t.qw;
    s.droneRoll = m_drone.roll();
    s.dronePitch = m_drone.pitch();
    s.droneYaw = m_drone.yaw();
    s.altitudeHoldActive = m_altHold.engaged;
    s.targetAltitude = m_altHold.targetAltitude;
    s.failsafeActive = m_failsafe.active;
    s.physicsMs = m_physics.profiler().stats().msSolver;
    s.bodiesAwake = m_physics.profiler().stats().bodiesAwake;
    // Altímetro por raycast hacia abajo. Mide desde el CENTRO del cuerpo, así
    // que se descuenta el radio para dar la altura libre bajo el dron: si no,
    // un dron posado marcaría 0,4 m de separación con el suelo.
    // El raycast también ve los tejados, que es la diferencia con leer la
    // altitud: sobre un edificio de 10 m marca 10, no 20.
    const physics::RaycastHit hit =
        m_physics.raycastClosest(m_drone.position(), {0, -1, 0}, 200.0f);
    s.groundDistance = hit.hit ? std::max(0.0f, hit.distance - m_gameCfg.droneRadius)
                               : std::max(0.0f, m_drone.position().y);
    // Distancia a la zona de aterrizaje mas cercana
    // El maximo por motor es una cuarta parte del empuje total del chasis.
    const float perMotorMax = m_gameCfg.maxThrust * 0.25f;
    for (int i = 0; i < 4; ++i)
        s.motorThrust[i] = perMotorMax > 0.0f ? clamp01(m_motorThrust[i] / perMotorMax) : 0.0f;

    s.nearestZoneDist = -1;
    for (const auto& zoneId : m_landingZoneBodies) {
        auto t = m_physics.getTransform(zoneId);
        float d = (t.position - m_drone.position()).length();
        if (s.nearestZoneDist < 0 || d < s.nearestZoneDist)
            s.nearestZoneDist = d;
        s.landingZonePositions.push_back(t.position);
    }
    return s;
}

}  // namespace drone
