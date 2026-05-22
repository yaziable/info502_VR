#version 460 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec4 FragPosLightSpace;

uniform sampler2D shadowMap;
uniform vec3  lightPos;
uniform vec3  viewPos;
uniform vec3  lightColor;
uniform vec3  objectColor;
uniform int   materialType;
uniform float sunIntensity;
uniform sampler2D grassTexture;
uniform sampler2D rockTexture;
uniform sampler2D snowTexture;
uniform samplerCube skybox;

// ── TA PARTIE : uniforms particules ──
uniform int   isParticle;

// Par :
uniform vec3 particleColor;
uniform float particleAlpha;

float ShadowCalculation(vec4 fragPosLightSpace);
float Noise(vec2 p);

void main() {

    // ── TA PARTIE : si c'est une particule ──
    // On affiche juste sa couleur + alpha, pas de calcul lumière

    if (isParticle == 1) {
    FragColor = vec4(particleColor, particleAlpha);
    return;
}

    // ── Code de ta camarade intact ──
    if (materialType == 3) {
        FragColor = vec4(objectColor * 1.35, 1.0);
        return;
    }

    if (materialType == 4) {
        FragColor = vec4(objectColor, 1.0);
        return;
    }

    if (materialType == 7) {
        vec3 smoothNormal = normalize(Normal);
        vec3 facetNormal = normalize(cross(dFdx(FragPos), dFdy(FragPos)));
        if (!gl_FrontFacing) facetNormal = -facetNormal;

        vec3 viewDir = normalize(FragPos - viewPos);
        vec3 toEye = normalize(viewPos - FragPos);
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 reflected = reflect(viewDir, facetNormal);
        vec3 refracted = refract(viewDir, smoothNormal, 1.0 / 1.35);
        vec3 reflectionColor = texture(skybox, reflected).rgb;
        vec3 refractionColor = texture(skybox, refracted).rgb;

        float shadow = ShadowCalculation(FragPosLightSpace);
        float diffuseFacet = max(dot(facetNormal, lightDir), 0.0);
        vec3 lightReflect = reflect(-lightDir, facetNormal);
        float sharpFlash = pow(max(dot(toEye, lightReflect), 0.0), 96.0);
        float broadGleam = pow(max(dot(toEye, lightReflect), 0.0), 14.0);
        float fresnel = pow(1.0 - max(dot(toEye, facetNormal), 0.0), 3.0);
        float visibleSun = max(sunIntensity, 0.08);

        vec3 crystalColor = mix(refractionColor, reflectionColor, 0.62 + fresnel * 0.25);
        crystalColor = mix(crystalColor, objectColor, 0.30);
        vec3 litTint = objectColor * lightColor * diffuseFacet * (1.0 - shadow) * visibleSun;
        vec3 reflectedLight = lightColor * (sharpFlash * 2.8 + broadGleam * 0.65 + fresnel * 0.75) * visibleSun;

        FragColor = vec4(crystalColor * (0.45 + 0.55 * visibleSun) + litTint + reflectedLight, 1.0);
        return;
    }

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    
    float shadow = ShadowCalculation(FragPosLightSpace);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 24.0);

    float variation = Noise(TexCoord * 55.0 + FragPos.xz * 0.09);
    vec3 color = objectColor;
    if (materialType == 0) {
        color = texture(grassTexture, TexCoord * 22.0).rgb;
    } else if (materialType == 1) {
        color = texture(rockTexture, TexCoord * 18.0).rgb;
    } else if (materialType == 2) {
        color = texture(snowTexture, TexCoord * 20.0).rgb;
    } else if (materialType == 5) {
        color = mix(vec3(0.24, 0.12, 0.05), vec3(0.42, 0.24, 0.10), variation);
    } else if (materialType == 6) {
        color = mix(vec3(0.04, 0.18, 0.07), vec3(0.10, 0.34, 0.12), variation);
    }

    float visibleSun = max(sunIntensity, 0.08);
    vec3 ambient  = color * mix(vec3(0.05, 0.07, 0.10),
                                vec3(0.25, 0.29, 0.24), visibleSun);
    vec3 diffuse  = color * lightColor * diff
                    * (1.0 - shadow) * visibleSun;
    vec3 specular = lightColor * spec
                    * (1.0 - shadow) * 0.18 * visibleSun;

    FragColor = vec4(ambient + diffuse + specular, 1.0);
}

float ShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0
     || projCoords.x < 0.0 || projCoords.x > 1.0
     || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 0.0;
    }

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = 0.005;
    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}

float Noise(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}
