#version 330

in vec3 fragPosition;
in vec3 fragNormal;
in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform vec3 ambientColor;
uniform vec3 viewPos;
uniform float fogDensity;

out vec4 finalColor;

void main() {
    vec4 tex = texture(texture0, fragTexCoord) * fragColor;

    vec3 N = normalize(fragNormal);
    vec3 L = normalize(-lightDir);
    float diff = max(dot(N, L), 0.0);

    // Iluminacion suave: ambiente alto + difusa
    vec3 ambient = ambientColor * 0.6;
    vec3 diffuse = lightColor * diff * 0.5;
    vec3 lit = (ambient + diffuse) * tex.rgb;

    // Niebla ligera
    float dist = length(viewPos - fragPosition);
    float fog = 1.0 - exp(-dist * fogDensity);
    vec3 fogColor = vec3(0.5, 0.6, 0.8);

    finalColor = vec4(mix(lit, fogColor, fog), tex.a);
}
