# Shadow Creatures

INFO-H-502 3D Graphics Project Report

Students: Aziable Yawa, TCHAMAKAM DJIOWA Murielle and Milong Irene

Git repository: https://github.com/yaziable/info502_VR.git

Project folder: `scene_shadow_creatures3/ShadowCreatures`

## 1. Introduction

Shadow Creatures is an interactive 3D scene where small creatures live according to light and shadow. The scene contains a procedural terrain, trees, a skybox, a moving sun, particles, shadow mapping, imported models, and interactive camera controls. The main idea is that shadows are not only visual: they also control the behavior of the creatures. If a creature is in shade, it grows. If it is exposed to sunlight, it shrinks. When it grows enough, it morphs into a permanent polygonal creature, sometimes becoming a reflective/refractive crystal.

The project was inspired by the lab exercises. We reused the main concepts from the course labs, then combined them into one scene:

- Lighting from the lighting lab.
- Textures from the texture lab.
- Camera movement from the camera/navigation lab.
- Model loading from the model loading lab.
- Cubemap and skybox from the environment mapping lab.
- Instancing for repeated objects.
- Particles and simple physics.
- Shadow mapping as an advanced feature.

## 2. Tools and Libraries Used

The project uses several libraries, each with a specific role.

| Tool / Library | Role in the project |
| --- | --- |
| OpenGL | Main graphics API. It renders meshes, textures, particles, skybox, lighting, and shadows. |
| GLFW | Creates the window and handles keyboard/mouse input. |
| GLAD | Loads modern OpenGL functions such as buffer, texture, framebuffer, and shader functions. |
| GLM | Provides vector and matrix mathematics for positions, transformations, camera, light space, and projections. |
| Assimp | Loads external `.obj` 3D models such as the bunny and sphere. |
| ImGui | Displays the small control/debug interface in the scene. |
| CMake | Builds the project and copies shaders/models into the build folder. |

Example initialization in `main.cpp`:

```cpp
glfwInit();
glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

GLFWwindow* window = glfwCreateWindow(
    1280, 720,
    "Shadow Creatures - 3D Graphics Project",
    NULL, NULL
);

glfwMakeContextCurrent(window);
gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
```

This creates the OpenGL window and loads the OpenGL functions.

## 3. General Scene Implementation

The scene is created around a main render loop in `main.cpp`. Each frame, the program updates inputs, camera, light, shadow map, creature logic, then renders everything.

Simplified frame structure:

```cpp
while (!glfwWindowShouldClose(window)) {
    float currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;

    processInput(window);
    camera.update(deltaTime);
    aiLights.update(deltaTime);

    // 1. Shadow pass
    // 2. Main render pass
    // 3. Creature update/render
    // 4. User interface
}
```

This structure is important because the project needs both rendering and simulation. The shadow pass creates data used by the shader and also by the creature logic.

## 4. Terrain: Procedural Mesh

The terrain is not loaded from a file. It is generated manually as a mesh in `Scene.h`.

We first create a regular grid in the XZ plane:

```cpp
int gridSize = 120;
float size = 90.0f;

for (int z = 0; z <= gridSize; ++z) {
    for (int x = 0; x <= gridSize; ++x) {
        float xPos = (float)x / gridSize * size - size / 2.0f;
        float zPos = (float)z / gridSize * size - size / 2.0f;
        float yPos = terrainHeight(xPos, zPos);
        glm::vec3 normal = terrainNormal(xPos, zPos);
    }
}
```

The terrain is a mesh because it is made of vertices and triangles. Each vertex has:

- position: `x`, `y`, `z`
- normal: used for lighting
- texture coordinate: used for textures

The height is computed mathematically:

```cpp
float terrainHeight(float x, float z) const {
    float center = std::sqrt(x * x + z * z);

    float ridgeA = std::exp(-std::pow((z + 8.0f) / 15.0f, 2.0f))
                 * (9.0f + 3.0f * std::sin(x * 0.16f));

    float ridgeB = std::exp(-std::pow((x - 12.0f) / 18.0f, 2.0f))
                 * (5.5f + 2.5f * std::cos(z * 0.13f));

    float rolling = std::sin(x * 0.18f) * std::cos(z * 0.14f) * 1.2f;

    float fineNoise = std::sin((x + z) * 0.65f)
                    * std::cos((x - z) * 0.31f) * 0.45f;

    float meadowFade = glm::smoothstep(5.0f, 22.0f, center);

    return (ridgeA + ridgeB) * meadowFade + rolling + fineNoise - 1.2f;
}
```

This function combines several mathematical shapes:

- `ridgeA` and `ridgeB` create mountain ridges.
- `sin` and `cos` create waves and hills.
- `fineNoise` creates smaller irregular details.
- `smoothstep` controls where mountains appear.

Then the grid cells are converted into triangles:

```cpp
indicesGreen.push_back(topLeft);
indicesGreen.push_back(bottomLeft);
indicesGreen.push_back(topRight);

indicesGreen.push_back(topRight);
indicesGreen.push_back(bottomLeft);
indicesGreen.push_back(bottomRight);
```

Each square of the grid becomes two triangles, which OpenGL can render.

## 5. Terrain Textures

The project implements textures, but instead of loading image files, we generate procedural textures in code.

There are three terrain textures:

- grass for low terrain
- rock for medium terrain
- snow for high terrain

The terrain triangles are separated into groups depending on height:

```cpp
if (avgY > 8.5f) {
    indicesSnow.push_back(...);
} else if (avgY > 3.2f) {
    indicesBrown.push_back(...);
} else {
    indicesGreen.push_back(...);
}
```

The textures are generated as RGB arrays:

```cpp
GLuint createPatternTexture(int kind) {
    const int texSize = 128;
    std::vector<unsigned char> data(texSize * texSize * 3);

    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            float fx = (float)x / (float)texSize;
            float fy = (float)y / (float)texSize;

            float n = std::sin(fx * 52.0f + fy * 17.0f)
                    * std::cos(fx * 19.0f - fy * 47.0f);
        }
    }
}
```

Then the data is uploaded to OpenGL:

```cpp
glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_RGB,
    texSize,
    texSize,
    0,
    GL_RGB,
    GL_UNSIGNED_BYTE,
    data.data()
);

glGenerateMipmap(GL_TEXTURE_2D);
```

In the fragment shader, we sample the correct texture:

```glsl
if (materialType == 0) {
    color = texture(grassTexture, TexCoord * 22.0).rgb;
} else if (materialType == 1) {
    color = texture(rockTexture, TexCoord * 18.0).rgb;
} else if (materialType == 2) {
    color = texture(snowTexture, TexCoord * 20.0).rgb;
}
```

## 6. Lighting

Lighting is inspired by the lighting lab. The shader computes ambient, diffuse, and specular lighting.

The light source is the sun. It changes position and color over time.

In `AILightController.cpp`, the sun position is updated with time:

```cpp
float time = (float)glfwGetTime();
float cycleDuration = 52.0f;
float phase = std::fmod(time / cycleDuration, 1.0f);

float orbitRadius = 45.0f;
lights[0].position = glm::vec3(
    std::cos(angle) * orbitRadius,
    std::sin(angle) * 30.0f,
    -18.0f
);
```

The light color changes between sunrise and noon:

```cpp
glm::vec3 sunriseColor(1.0f, 0.48f, 0.20f);
glm::vec3 noonColor(1.0f, 0.96f, 0.82f);

return glm::mix(
    sunriseColor,
    noonColor,
    glm::smoothstep(0.35f, 0.85f, intensity)
);
```

In the fragment shader, we compute:

```glsl
vec3 norm = normalize(Normal);
vec3 lightDir = normalize(lightPos - FragPos);

float diff = max(dot(norm, lightDir), 0.0);

vec3 viewDir = normalize(viewPos - FragPos);
vec3 reflectDir = reflect(-lightDir, norm);
float spec = pow(max(dot(viewDir, reflectDir), 0.0), 24.0);
```

Then the final color is:

```glsl
vec3 ambient  = color * mix(vec3(0.05, 0.07, 0.10),
                            vec3(0.25, 0.29, 0.24), visibleSun);

vec3 diffuse  = color * lightColor * diff
              * (1.0 - shadow) * visibleSun;

vec3 specular = lightColor * spec
              * (1.0 - shadow) * 0.18 * visibleSun;

FragColor = vec4(ambient + diffuse + specular, 1.0);
```

So the basic lighting features are:

- ambient light
- diffuse light
- specular highlight
- dynamic light position
- dynamic light color
- shadow influence

## 7. Shadow Mapping

Shadow mapping is one of our advanced features.

The idea is to render the scene once from the sun's point of view and store only depth. This depth texture is the shadow map. Later, when rendering from the camera, we compare a fragment's depth from the light with the stored depth. If the fragment is farther away, another object is blocking the light, so it is in shadow.

The shadow map is created in `main.cpp`:

```cpp
GLuint shadowMapFBO, shadowMap;

glGenFramebuffers(1, &shadowMapFBO);
glGenTextures(1, &shadowMap);

glBindTexture(GL_TEXTURE_2D, shadowMap);
glTexImage2D(
    GL_TEXTURE_2D,
    0,
    GL_DEPTH_COMPONENT,
    2048,
    2048,
    0,
    GL_DEPTH_COMPONENT,
    GL_FLOAT,
    NULL
);
```

It is attached to a framebuffer:

```cpp
glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
glFramebufferTexture2D(
    GL_FRAMEBUFFER,
    GL_DEPTH_ATTACHMENT,
    GL_TEXTURE_2D,
    shadowMap,
    0
);

glDrawBuffer(GL_NONE);
glReadBuffer(GL_NONE);
```

We use a `lightSpaceMatrix` to render from the sun's point of view:

```cpp
glm::mat4 lightProjection = glm::ortho(
    -42.0f, 42.0f,
    -42.0f, 42.0f,
    near_plane,
    far_plane
);

glm::mat4 lightView = glm::lookAt(
    lightPos,
    glm::vec3(0.0f),
    glm::vec3(0.0, 1.0, 0.0)
);

return lightProjection * lightView;
```

During the shadow pass:

```cpp
glViewport(0, 0, 2048, 2048);
glBindFramebuffer(GL_FRAMEBUFFER, shadowMapFBO);
glClear(GL_DEPTH_BUFFER_BIT);

shadowShader.use();
shadowShader.setMat4("lightSpaceMatrix", aiLights.getLightSpaceMatrix());

scene.renderShadows(shadowShader);
scene.renderTreeShadows(shadowShader);
bunnyModel.renderShadow(shadowShader, ...);
sphereModel.renderShadow(shadowShader, ...);
```

In the fragment shader, we use the shadow map:

```glsl
float ShadowCalculation(vec4 fragPosLightSpace) {
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float closestDepth = texture(shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float bias = 0.005;

    return currentDepth - bias > closestDepth ? 1.0 : 0.0;
}
```

This returns:

- `0.0` if the fragment is lit
- `1.0` if the fragment is in shadow

We do not use a precomputed light map. The lighting is computed dynamically in the shader, and the shadow map tells the shader which fragments receive direct sunlight.

## 8. Skybox and Cubemap

The cubemap is the large cube surrounding the scene. The terrain, trees, creatures, sun, and models are inside it. It acts as the background sky.

OpenGL provides the cubemap texture type:

```cpp
GL_TEXTURE_CUBE_MAP
```

But we create the cubemap content ourselves. In `Scene::initSkybox`, we generate six texture faces:

```cpp
for (int face = 0; face < 6; ++face) {
    std::vector<unsigned char> data(texSize * texSize * 3);

    for (int y = 0; y < texSize; ++y) {
        for (int x = 0; x < texSize; ++x) {
            float u = (float)x / (float)(texSize - 1);
            float v = (float)y / (float)(texSize - 1);
            glm::vec3 color = skyboxColor(face, u, v);
        }
    }

    glTexImage2D(
        GL_TEXTURE_CUBE_MAP_POSITIVE_X + face,
        0,
        GL_RGB,
        texSize,
        texSize,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        data.data()
    );
}
```

The skybox is rendered with the camera rotation but without camera translation:

```cpp
shader.setMat4("view", glm::mat4(glm::mat3(view)));
```

This makes the skybox feel infinitely far away.

The cubemap is also reused for the crystal reflection/refraction effect.

## 9. Trees and Instancing

Trees are generated in code. Each tree is made of:

- a trunk mesh
- a leaves mesh

Instead of creating separate geometry for every tree, we use instancing. The tree positions are stored in an instance buffer:

```cpp
glGenBuffers(1, &treeInstanceVBO);
glBindBuffer(GL_ARRAY_BUFFER, treeInstanceVBO);
glBufferData(
    GL_ARRAY_BUFFER,
    treePositions.size() * sizeof(glm::vec3),
    treePositions.data(),
    GL_STATIC_DRAW
);
```

The instance offset is stored as vertex attribute 3:

```cpp
glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
glEnableVertexAttribArray(3);
glVertexAttribDivisor(3, 1);
```

The divisor means this attribute changes once per instance, not once per vertex.

In the vertex shader:

```glsl
FragPos = vec3(model * vec4(aPos, 1.0));

if (useInstancing == 1) {
    FragPos += aInstanceOffset;
}
```

Then the trees are drawn using:

```cpp
glDrawElementsInstanced(
    GL_TRIANGLES,
    trunkIndexCount,
    GL_UNSIGNED_INT,
    0,
    (GLsizei)treePositions.size()
);
```

This is more efficient than drawing each tree individually and demonstrates instancing from the course.

## 10. Imported Models

The bunny and sphere are imported models. They are also meshes, but unlike the terrain, their mesh data comes from external `.obj` files.

The project uses Assimp in `LoadedModel.cpp`:

```cpp
Assimp::Importer importer;
const aiScene* scene = importer.ReadFile(
    path,
    aiProcess_Triangulate |
    aiProcess_GenSmoothNormals |
    aiProcess_JoinIdenticalVertices |
    aiProcess_ImproveCacheLocality
);
```

Assimp reads:

- vertex positions
- normals
- texture coordinates
- triangle indices

Then we upload the model mesh to OpenGL buffers:

```cpp
glGenVertexArrays(1, &result.VAO);
glGenBuffers(1, &result.VBO);
glGenBuffers(1, &result.EBO);

glBindVertexArray(result.VAO);
glBindBuffer(GL_ARRAY_BUFFER, result.VBO);
glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, result.EBO);
glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
```

The models are created in `main.cpp`:

```cpp
LoadedModel bunnyModel("models/bunny_small.obj");
LoadedModel sphereModel("models/sphere_smooth.obj");
```

They are rendered as scene props and also included in the shadow pass:

```cpp
bunnyModel.render(creatureShader, position, scale, color, 4);
sphereModel.render(creatureShader, position, scale, color, 4);
```

If they do not appear, the first thing to check is that the executable is launched from a build folder containing:

```text
models/bunny_small.obj
models/sphere_smooth.obj
```

## 11. Camera and User Interaction

The camera uses a view matrix built with `glm::lookAt`:

```cpp
glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(Position, Position + Front, Up);
}
```

The projection is perspective:

```cpp
glm::mat4 Camera::getProjectionMatrix() const {
    return glm::perspective(
        glm::radians(Zoom),
        1280.0f / 720.0f,
        0.1f,
        100.0f
    );
}
```

Controls:

- `W/A/S/D`: move camera
- right mouse: rotate camera
- middle mouse: pan camera
- scroll: zoom
- left mouse: grab a creature

Mouse dragging uses a ray from the camera through the mouse position:

```cpp
glm::mat4 invVP = glm::inverse(
    camera.getProjectionMatrix() * camera.getViewMatrix()
);

glm::vec4 nearPt = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
glm::vec4 farPt  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);

glm::vec3 rayDir = glm::normalize(glm::vec3(farPt) - glm::vec3(nearPt));
```

If this ray passes close enough to a creature, the creature becomes selected and can be dragged.

## 12. Game Logic: Shadow Creatures

The main game logic is implemented in `ShadowCreature::update` and in the shade detection functions in `main.cpp`.

Each creature has:

- a position
- a size
- a morphed state
- a crystal state
- particles

The rule is:

- in shade: grow
- in sunlight: shrink
- if size reaches max: morph
- if size becomes too small: disappear

Code:

```cpp
if (inShade) {
    size += growthSpeed * deltaTime * 0.08f;
} else {
    size -= growthSpeed * deltaTime * 0.2f;
}

if (size >= maxCreatureSize) {
    size = maxCreatureSize;
    morphToCreature(sunIntensity, lightColor);
}
```

The main function deciding shade is:

```cpp
bool isCreatureInShade(...) {
    if (creature.isMorphed) return false;
    if (sunIntensity < 0.18f) return true;

    bool inShadowMap = isCreatureInShadowMap(...);

    return inShadowMap
        || isTerrainBlockingSun(...)
        || isPolygonBlockingSun(...);
}
```

This means a creature is in shade if at least one condition is true:

- the shadow map says it is hidden from the sun
- terrain blocks the sun
- a morphed polygon creature casts a shadow over it

## 13. Shadow Map Used for Game Logic

At first, one possible idea is to cast a ray from each creature to the sun and check if something blocks it. That is simple conceptually, but hard to do for the whole scene because we would need ray intersection with terrain, trees, imported models, and creatures.

Instead, we reuse the shadow map that the GPU already generated.

The creature position is transformed into light space:

```cpp
glm::vec4 worldPos = glm::vec4(position, 1.0f);
glm::vec4 lightSpacePos = lightSpaceMatrix * worldPos;
```

Then we convert it to shadow texture coordinates:

```cpp
glm::vec3 projCoords = glm::vec3(lightSpacePos) / lightSpacePos.w;
projCoords = projCoords * 0.5f + 0.5f;
```

We find the corresponding shadow map pixel:

```cpp
int px = std::clamp((int)(projCoords.x * width), 0, width - 1);
int py = std::clamp((int)(projCoords.y * height), 0, height - 1);
```

Then compare the creature depth with the stored closest depth:

```cpp
float closestDepth = shadowDepth[py * width + px];
float bias = 0.005f;

return projCoords.z - bias > closestDepth;
```

If the creature depth is larger than the shadow map depth, then another object is closer to the sun. Therefore the creature is behind an obstacle and is in shadow.

We still keep extra tests:

```cpp
isTerrainBlockingSun(...)
isPolygonBlockingSun(...)
```

These make the game logic more robust and allow morphed creatures to create simplified protective shadow areas.

## 14. Morphing: Blob to Polygon or Crystal

When a blob reaches maximum size, it morphs:

```cpp
void ShadowCreature::morphToCreature(float sunIntensity, glm::vec3 lightColor) {
    isMorphed = true;
    isCrystal = (rand() % 3) == 0;
    spawnParticles(sunIntensity, lightColor, 3.0f);
}
```

This does three things:

1. Sets `isMorphed = true`.
2. Randomly chooses if the creature is a crystal.
3. Spawns particles for the morph effect.

Rendering depends on `isCrystal`:

```cpp
shader.setInt("materialType", isCrystal ? 7 : 4);

shader.setVec3(
    "objectColor",
    isCrystal ? glm::vec3(0.36f, 0.92f, 1.0f)
              : glm::vec3(0.08f, 0.07f, 0.12f)
);
```

So:

- normal morphed creature: opaque dark polygon
- crystal creature: blue reflective/refractive polygon

The polygon mesh is an icosahedron-like shape created in `initPolyMesh`.

## 15. Reflection and Refraction on Crystal Creatures

The crystal material is an advanced effect. It is implemented in the fragment shader when:

```glsl
materialType == 7
```

Reflection:

```glsl
vec3 viewDir = normalize(FragPos - viewPos);
vec3 reflected = reflect(viewDir, facetNormal);
vec3 reflectionColor = texture(skybox, reflected).rgb;
```

Refraction:

```glsl
vec3 refracted = refract(viewDir, smoothNormal, 1.0 / 1.35);
vec3 refractionColor = texture(skybox, refracted).rgb;
```

The shader mixes both using a Fresnel-like term:

```glsl
float fresnel = pow(1.0 - max(dot(toEye, facetNormal), 0.0), 3.0);

vec3 crystalColor = mix(
    refractionColor,
    reflectionColor,
    0.62 + fresnel * 0.25
);
```

Meaning:

- direct view: more refraction
- grazing angle: more reflection

The crystal uses the cubemap as its reflected/refracted environment.

## 16. Particles

Particles are used when a creature morphs. Each particle has:

- position
- velocity
- color
- lifetime
- size

Particles are spawned here:

```cpp
void ShadowCreature::spawnParticles(float sunIntensity, glm::vec3 lightColor, float speed) {
    for (int i = 0; i < 40; i++) {
        Particle p;
        p.position = position;

        float vx = (rand() % 200 - 100) / 100.0f;
        float vy = (rand() % 200) / 100.0f;
        float vz = (rand() % 200 - 100) / 100.0f;

        p.velocity = glm::vec3(vx, vy, vz) * speed;
        particles.push_back(p);
    }
}
```

They are updated every frame:

```cpp
p.position += p.velocity * deltaTime;
p.velocity.y -= 9.8f * deltaTime;
p.life -= deltaTime;
p.color.a = glm::max(0.0f, p.life);
```

Particles are rendered with additive blending:

```cpp
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE);
glDepthMask(GL_FALSE);
```

This makes them look glowing.

## 17. Collision and Simple Physics

Collision is implemented between particles in `ShadowCreature::updateParticles`.

Each particle is approximated as a small sphere. We test all particle pairs:

```cpp
for (size_t i = 0; i < particles.size(); ++i) {
    for (size_t j = i + 1; j < particles.size(); ++j) {
        glm::vec3 diff = particles[j].position - particles[i].position;
        float dist2 = glm::dot(diff, diff);
    }
}
```

If two particles are too close:

```cpp
if (dist2 < minDist2 && dist2 > 1e-6f) {
    glm::vec3 n = diff / std::sqrt(dist2);
}
```

We compute an impulse:

```cpp
float impulse = glm::dot(
    particles[i].velocity - particles[j].velocity,
    n
);
```

Then apply a bounce:

```cpp
float restitution = 0.6f;

particles[i].velocity -= (1.0f + restitution) * 0.5f * impulse * n;
particles[j].velocity += (1.0f + restitution) * 0.5f * impulse * n;
```

Finally, we separate overlapping particles:

```cpp
float overlap = minDist - std::sqrt(dist2);
particles[i].position -= 0.5f * overlap * n;
particles[j].position += 0.5f * overlap * n;
```

This produces a more physical particle burst: particles bounce away from each other instead of passing through each other.

## 18. User Interface

ImGui is used for a small control panel:

```cpp
ImGui::Begin("Shadow Creatures Control");
ImGui::Text("Sun cycle: %.0f%% daylight", sunIntensity * 100.0f);
ImGui::Text("Creatures: %d", (int)creatures.size());
ImGui::Text("Controls:");
ImGui::Text("  Clic droit + souris : orienter camera");
ImGui::Text("  Clic milieu + souris : pan camera");
ImGui::Text("  Clic gauche : attraper creature");
ImGui::Text("  W/A/S/D : deplacer camera");
ImGui::Text("  Scroll : zoom");
ImGui::End();
```

This helps during demonstration because the user can see the number of creatures and the daylight percentage.

## 19. Requirement Summary

| Requirement level | Feature | Implemented how |
| --- | --- | --- |
| Basic | Lighting | Ambient, diffuse, specular lighting in `shadow_creature.frag` |
| Basic | Textures | Procedural grass, rock, snow textures |
| Basic | Loaded models | Bunny and sphere loaded with Assimp |
| Basic | Moving objects | Moving sun, growing/shrinking creatures, particles |
| Basic | Free camera | Keyboard and mouse camera controls |
| Intermediate | Particles | Morphing explosion particles |
| Intermediate | Cubemap | Procedural skybox |
| Intermediate | Instancing | Trees rendered with instance offsets |
| Intermediate | Game logic | Creatures grow in shadow and shrink in light |
| Advanced | Shadows | Shadow mapping with depth framebuffer |
| Advanced | Collision/physics | Particle-particle collision and bounce |
| Advanced | Reflection/refraction | Crystal creatures sample the cubemap |
| Additional | Procedural geometry | Terrain, trees, sun mesh, blob mesh, polygon mesh |

## 20. Link With Theory

The strongest theory link is shadows, especially shadow mapping.

In theory, a point is lit if it is visible from the light. Shadow mapping approximates this by rendering a depth image from the light's point of view. During normal rendering, each fragment is transformed into the same light space and compared with the stored depth.

Advantages:

- Works with many objects.
- Efficient on the GPU.
- Easy to combine with normal shading.
- Can be reused for game logic.

Limitations:

- Resolution can create jagged shadows.
- Needs bias to avoid self-shadowing artifacts.
- The light projection must cover the scene.
- Basic shadow mapping creates hard shadows unless filtering is added.

Our implementation uses a 2048 by 2048 depth texture, a light-space matrix, and a small bias to reduce artifacts.

## 21. Possible Additional Feature

A good future improvement would be normal mapping.

Normal mapping would add small surface details to grass, rock, snow, and crystal materials without increasing mesh complexity.

Implementation plan:

1. Generate or load normal maps.
2. Add tangent and bitangent vectors to the mesh data.
3. Build a TBN matrix in the shader.
4. Sample the normal map.
5. Use the modified normal in the lighting calculation.

This would improve terrain detail while fitting naturally with the existing texture and lighting system.

## 22. How to Launch

Dependencies:

- CMake 3.20 or newer.
- C++ compiler.
- OpenGL-compatible GPU/driver.
- Internet access during first CMake build if Assimp/ImGui need to be fetched.

Build:

```bash
cd scene_shadow_creatures3/ShadowCreatures
cmake -S . -B build-codex
cmake --build build-codex
```

Run:

```bash
cd build-codex
./ShadowCreatures.exe
```

The executable must be run from a folder containing:

```text
shaders/
models/
```

Controls:

- `W/A/S/D`: move camera
- right mouse: rotate camera
- middle mouse: pan camera
- mouse wheel: zoom
- left click: grab creature
- `Esc`: quit

## 23. Defense Notes

Short project explanation:

> Shadow Creatures is an OpenGL scene where creatures react to shadows. We implemented the required basics: lighting, textures, model loading, moving objects, and camera navigation. We also added particles, cubemap skybox, instancing, and game logic. Our main advanced feature is shadow mapping, which is used both visually and for creature behavior.

If asked how the terrain is made:

> The terrain is a procedural mesh. We create a grid, compute the height of each vertex with a mathematical height function, then connect the grid cells into triangles.

If asked if the models are meshes:

> Yes. Everything rendered is ultimately a mesh. The terrain and trees are generated in code, while the bunny and sphere meshes are loaded from `.obj` files using Assimp.

If asked how the shadow map works:

> We first render the scene from the sun's point of view into a depth texture. During the normal render pass, we transform each fragment into light space and compare its depth with the stored depth. If it is farther, it is in shadow.

If asked how creatures know they are in shadow:

> We reuse the shadow map. The creature position is transformed by the light-space matrix, then compared against the shadow map depth. We also use terrain and polygon shadow tests to make the game logic more robust.

If asked about collision:

> Collision is implemented between particles. Each particle is treated as a small sphere. If two particles overlap, we apply a bounce impulse and separate them.

If asked about reflection:

> Reflection is implemented on crystal creatures. The shader computes reflected and refracted directions and samples the cubemap skybox. The result is mixed with a Fresnel-like term.

## 24. Conclusion

The project meets the basic course requirements and includes several intermediate and advanced features. The scene is built from procedural meshes, generated textures, imported models, dynamic lighting, and a cubemap skybox. The most important advanced feature is shadow mapping, because it affects both rendering and gameplay. The creatures use the shadow information to grow, shrink, morph, and create particles, turning the graphics techniques into a small interactive ecosystem.

