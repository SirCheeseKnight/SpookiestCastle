// THIS IS THE FILE YOU MUST START FROM!

// This has been adapted from the Vulkan tutorial
#include <sstream>
#include <vector>
#include <fstream>
#include <random>
#include <unordered_map>

#include <json.hpp>

#include "modules/Starter.hpp"
#include "modules/TextMaker.hpp"
#include "modules/Scene.hpp"

// The uniform buffer object used in this example
struct UniformBufferObject {
  alignas(16) glm::mat4 mvpMat;
  alignas(16) glm::mat4 mMat;
  alignas(16) glm::mat4 normalMat;
  // x: world-space texture scale, y: enable planar tiling
  alignas(16) glm::vec4 surfaceParams;
};

struct GlobalUniformBufferObject {
  alignas(16) glm::vec3 lightDir;
  alignas(16) glm::vec4 lightColor;
  alignas(16) glm::vec3 eyePos;
  alignas(16) glm::vec4 pointLightPos[6];
  alignas(16) glm::vec4 pointLightColor[6];
};

struct Vertex {
  glm::vec3 pos;
  glm::vec3 normal;
  glm::vec2 UV;
};

struct SkyUniformBufferObject {
  alignas(16) glm::mat4 inverseViewProjection;
  alignas(16) glm::vec2 resolution;
};

// MAIN !

class Skeleton26ReplaceName : public BaseProject {
protected:
  // Here you list all the Vulkan objects you need:
  
  // Descriptor Layouts [what will be passed to the shaders]
  DescriptorSetLayout DSLlocal, DSLglobal, DSLsky;
  
  // Vertex formants, Pipelines [Shader couples] and Render passes
  VertexDescriptor VD, VDsky;
  RenderPass RP;
  Pipeline P, Psky;
  
  // Models, textures and Descriptors (values assigned to the uniforms)
  DescriptorSet DSglobal, DSsky;
  
  // To support loading assets from a scene.json file
  Scene SC;
  std::vector<VertexDescriptorRef>  VDRs;
  std::vector<TechniqueRef> PRs;
  
  // to provide textual feedback
  TextMaker txt;
  
  // Other application parameters
  float Ar;	// Aspect ratio
  
  glm::mat4 ViewPrj;
  glm::mat4 View;

  // Add these lines to keep track of camera state:
  glm::vec3 camPos = glm::vec3(0.0f, 2.5f, 80.0f); // In front of the castle
  float camYaw = glm::radians(180.0f);              // Look toward the castle (-Z)
  float camPitch = glm::radians(-10.0f);            // Include the castle and sky
  // Rock physics state
  glm::vec3 rock1Pos = glm::vec3(115.0f, 1080.0f, 10.0f);
  glm::vec3 rock2Pos = glm::vec3(90.0f, 1080.0f, 10.0f);
  float rock1VelY = 0.0f;
  float rock2VelY = 0.0f;
  bool rock1Grounded = false;
  bool rock2Grounded = false;
  float rock1Angle = 0.0f;
  float rock2Angle = 0.0f;
  float velocity_y = 0;
  float gravity = -19.6f;      // Downward acceleration (m/s^2)
  float jumpImpulse = 9.0f;    // Initial upward velocity when jumping
  float playerHeight = 2.5f; // Distance from player center to feet
  bool isGrounded = false;
  int graveIdCounter = 0;
  bool rockStart = false;
  bool rockStop = false;
  std::vector<float> ghostDirections;
  std::vector<float> ghostSpeeds;
  std::vector<float> initialGhostDirections;
  std::vector<bool> activeGhosts;
  std::vector<bool> brokenGraves;
  std::vector<bool> collisionDisabled;
  std::vector<glm::mat4> initialGraveTransforms;
  std::vector<glm::mat4> initialBrokenGraveTransforms;
  std::vector<glm::mat4> initialGhostTransforms;

  enum class GameState { Playing, Won, Lost };
  GameState gameState = GameState::Playing;
  bool hasPotion = false;
  bool potionAvailable = true;
  bool potionTransformSaved = false;
  bool fireWasPressed = false;
  bool replayWasPressed = false;
  int remainingGhosts = 0;
  int spawnedGhosts = 0;
  glm::mat4 initialPotionTransform = glm::mat4(1.0f);

  // Maps string ID -> array index inside SC.TI[0].I
  std::unordered_map<std::string, int> instanceIndexMap;

  void mapInstanceIndices(const nlohmann::json& sceneData) {
    auto& elements = sceneData["instances"][0]["elements"];
    for (int i = 0; i < elements.size(); ++i) {
      std::string id = elements[i]["id"];
      instanceIndexMap[id] = i; // Save exact position in the SC array
    }
  }
  
  // Here you set the main application parameters
  void setWindowParameters() {
    // window size, titile and initial background
    windowWidth = 1280;
    windowHeight = 720;
    windowTitle = "The Spookiest of Castles";
    windowResizable = GLFW_TRUE;
    
    // Initial aspect ratio
    Ar = 16.0f / 9.0f;
  }
  
  // What to do when the window changes size
  void onWindowResize(int w, int h) {
    std::cout << "Window resized to: " << w << " x " << h << "\n";
    Ar = (float)w / (float)h;
    // Update Render Pass
    RP.width = w;
    RP.height = h;
    
    // updates the textual output
    txt.resizeScreen(w, h);
  }
  
  // Here you load and setup all your Vulkan Models and Texutures.
  // Here you also create your Descriptor set layouts and load the shaders for the pipelines
  void localInit() {
    // Descriptor Layouts [what will be passed to the shaders]
    DSLlocal.init(this, {
	// this array contains the binding:
	// first  element : the binding number
	// second element : the type of element (buffer or texture)
	// third  element : the pipeline stage where it will be used
	{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(UniformBufferObject), 1},
	{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
      });
    DSLglobal.init(this, {
	// this array contains the binding:
	// first  element : the binding number
	// second element : the type of element (buffer or texture)
	// third  element : the pipeline stage where it will be used
	{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
      });
    DSLsky.init(this, {
	{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(SkyUniformBufferObject), 1}
      });
    VD.init(this, {
	{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
      }, {
	{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
	 sizeof(glm::vec3), POSITION},
	{0, 1, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal),
	 sizeof(glm::vec3), NORMAL},
	{0, 2, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
	 sizeof(glm::vec2), UV}
      });

    // The sky uses gl_VertexIndex, so it needs no vertex buffer or attributes.
    VDsky.init(this, {}, {});
    
    // initializes the render passes
    RP.init(this);
    // Dark fallback color underneath the rendered night sky.
    RP.properties[0].clearValue = {0.003f, 0.006f, 0.02f, 1.0f};

    
    // Pipelines [Shader couples]
    // The last array, is a vector of pointer to the layouts of the sets that will
    // be used in this pipeline. The first element will be set 0, and so on..
    
    P.init(this, &VD, "shaders/toChangeSimplePos.vert.spv",
	   "shaders/toChangeBlinnFromPos.frag.spv",
	   {&DSLglobal, &DSLlocal});

    Psky.init(this, &VDsky, "shaders/nightSky.vert.spv",
	     "shaders/nightSky.frag.spv", {&DSLsky});
    Psky.setCullMode(VK_CULL_MODE_NONE);
    Psky.setCompareOp(VK_COMPARE_OP_LESS_OR_EQUAL);
    
    
    // sets the size of the Descriptor Set Pool (it MUST be done before loading the scene)
    DPSZs.uniformBlocksInPool = 3;
    DPSZs.texturesInPool = 1;
    DPSZs.setsInPool = 3;
    
    // to support scene
    VDRs.resize(1);
    VDRs[0].init("VDposUV",  &VD);
    
    PRs.resize(1);
    PRs[0].init("BlinnPos", {
	{&P, {//Pipeline and DSL for the main pass
	    /*DSLglobal*/{},
	      /*DSLlocal*/{
	      /*t0*/{true,  0, {}}
	    }
	  }
	}
      }, /*TotalNtextures*/1, &VD);
    
    // 1. Setup Random Number Generators
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> numGravesDist(1, 3);
    std::uniform_real_distribution<float> xPosDist0(80.0f, 90.0f);
    std::uniform_real_distribution<float> xPosDist2(95.0f, 105.0f);
    std::uniform_real_distribution<float> xPosDist1(110.0f, 120.0f);
    std::uniform_real_distribution<float> yRotDist(-45.0f, 45.0f);
    std::uniform_real_distribution<float> currSpeed(6.0f, 9.0f);

    // 2. Load the base scene JSON
    std::ifstream inFile("assets/scenes/scene.json");
    nlohmann::json sceneData;
    inFile >> sceneData;
    inFile.close();

    // 3. Procedurally generate graves across 22 steps
    int totalSteps = 22;


    for (int step = 0; step < totalSteps; ++step) {
      float currentY = 1036.5f + (1.33f * step);
      float currentZ = 60.33f - (1.66f * step);
      float randomX = 0.0f;
      int gravesOnThisStep = numGravesDist(gen);

      for (int g = 0; g < gravesOnThisStep; ++g) {
        int graveID = graveIdCounter++;
        if (g==0)
        {
           randomX = xPosDist0(gen);
        }
        else if (g==1)
        {
          randomX = xPosDist1(gen);
        }
        else
        {
          randomX = xPosDist2(gen);
        }

        float randomRotY = yRotDist(gen);

        // 1. Create an empty JSON object
        nlohmann::json graveInst = nlohmann::json::object();
        nlohmann::json brokenGraveInst = nlohmann::json::object();
        nlohmann::json ghostInst = nlohmann::json::object();

        // 2. Assign strings explicitly
        graveInst["id"] = "grave_auto_" + std::to_string(graveID);
        graveInst["model"] = "grave_short";
        brokenGraveInst["id"] = "broken_grave_auto_" + std::to_string(graveID);
        brokenGraveInst["model"] = "broken_grave_short";
        ghostInst["id"] = "ghost_auto_" + std::to_string(graveID);
        ghostInst["model"] = "ghost";

        // 3. Assign arrays explicitly using standard C++ vectors so JSON knows exactly what they are
        graveInst["texture"] = std::vector<std::string>{"dungeon"};std::vector<float>{0.0f, 0.0f, 0.0f};
        graveInst["translate"] = std::vector<float>{randomX, currentY, currentZ};
        graveInst["scale"] = std::vector<float>{1.5f, 1.5f, 1.5f};
        graveInst["eulerAngles"] = std::vector<float>{0.0f, randomRotY, 0.0f};

        brokenGraveInst["texture"] = graveInst["texture"];
        brokenGraveInst["translate"] = std::vector<float>{randomX + 0.1f, currentY - 0.7f, currentZ - 0.4f};
        brokenGraveInst["scale"] = std::vector<float>{0.001f, 0.001f, 0.001f};
        brokenGraveInst["eulerAngles"] = graveInst["eulerAngles"];

        ghostInst["texture"] = std::vector<std::string>{"dungeon"};
        ghostInst["translate"] = std::vector<float>{randomX, currentY + 1.2f, currentZ};
        ghostInst["scale"] = std::vector<float>{0.001f, 0.001f, 0.001f};

        if (randomX > 100.0f) {
          ghostInst["eulerAngles"] = std::vector<float>{0.0f, -90.0f, 0.0f};
          ghostDirections.push_back(-1.0f); // Initially moving towards decreasing X
        } else {
          ghostInst["eulerAngles"] = std::vector<float>{0.0f, 90.0f, 0.0f};
          ghostDirections.push_back(1.0f);  // Initially moving towards increasing X
        }

        // 4. Append to the instances array
        sceneData["instances"][0]["elements"].push_back(graveInst);

        sceneData["instances"][0]["elements"].push_back(brokenGraveInst);

        sceneData["instances"][0]["elements"].push_back(ghostInst);

        activeGhosts.push_back(false);
        ghostSpeeds.push_back(currSpeed(gen));
      }
    }

    mapInstanceIndices(sceneData);

    // 4. Save to a temporary file
    std::string generatedScenePath = "assets/scenes/scene_generated.json";
    std::ofstream outFile(generatedScenePath);
    outFile << std::setw(4) << sceneData << std::endl;
    outFile.close();

    // 5. Load the newly generated scene instead of the base one
    if(SC.init(this, 1, VDRs, PRs, generatedScenePath) != 0) {
      std::cout << "ERROR LOADING THE SCENE\n";
      exit(0);
    }

    brokenGraves.assign(graveIdCounter, false);
    initialGhostDirections = ghostDirections;
    collisionDisabled.assign(SC.TI[0].InstanceCount, false);
    initialGraveTransforms.resize(graveIdCounter);
    initialBrokenGraveTransforms.resize(graveIdCounter);
    initialGhostTransforms.resize(graveIdCounter);

    for (int i = 0; i < graveIdCounter; ++i) {
      int graveIdx = instanceIndexMap["grave_auto_" + std::to_string(i)];
      int brokenIdx = instanceIndexMap["broken_grave_auto_" + std::to_string(i)];
      int ghostIdx = instanceIndexMap["ghost_auto_" + std::to_string(i)];
      initialGraveTransforms[i] = SC.TI[0].I[graveIdx].Wm;
      initialBrokenGraveTransforms[i] = SC.TI[0].I[brokenIdx].Wm;
      initialGhostTransforms[i] = SC.TI[0].I[ghostIdx].Wm;
    }

    if (instanceIndexMap.count("hidden_room_potion")) {
      initialPotionTransform = SC.TI[0].I[instanceIndexMap["hidden_room_potion"]].Wm;
      potionTransformSaved = true;
    }
    
    // initializes the textual output
    txt.init(this, windowWidth, windowHeight);
    
    // submits the main command buffer
    submitCommandBuffer("main", 0, populateCommandBufferAccess, this);
    
    // Prepares for showing the FPS count
    txt.print(1.0f, 1.0f, "FPS:",1,"CO",false,false,true,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
    
  }
  
  // Here you create your pipelines and Descriptor Sets!
  void pipelinesAndDescriptorSetsInit() {
    // creates the render passes
    RP.create();
    
    // This creates a new pipeline (with the current surface), using its shaders for the provided render pass
    P.create(&RP);
    Psky.create(&RP);
    
    DSglobal.init(this, &DSLglobal, {});
    DSsky.init(this, &DSLsky, {});
    
    // Here you define the data set
    // If the scene has textures coming from a render pass, the corresponding element of the technique must be
    // updated before calling SC.pipelinesAndDescriptorSetsInit();
    
    SC.pipelinesAndDescriptorSetsInit();
    txt.pipelinesAndDescriptorSetsInit();
  }
  
  // Here you destroy your pipelines and Descriptor Sets!
  void pipelinesAndDescriptorSetsCleanup() {
    P.cleanup();
    Psky.cleanup();
    
    RP.cleanup();
    
    DSglobal.cleanup();
    DSsky.cleanup();
    
    SC.pipelinesAndDescriptorSetsCleanup();
    txt.pipelinesAndDescriptorSetsCleanup();
  }
  
  // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
  // You also have to destroy the pipelines
  void localCleanup() {
    DSLlocal.cleanup();
    DSLglobal.cleanup();
    DSLsky.cleanup();
    
    P.destroy();
    Psky.destroy();
    VDsky.cleanup();
    
    RP.destroy();
    
    SC.localCleanup();
    txt.localCleanup();
    //std::filesystem::remove("assets/scenes/scene_generated.json");
  }
  
  // Here it is the creation of the command buffer:
  // You send to the GPU all the objects you want to draw,
  // with their buffers and textures
  static void populateCommandBufferAccess(VkCommandBuffer commandBuffer, int currentImage, void *Params) {
    // Simple trick to avoid having always 'T->'
    // in che code that populates the command buffer!
    Skeleton26ReplaceName *T = (Skeleton26ReplaceName *)Params;
    T->populateCommandBuffer(commandBuffer, currentImage);
  }
  
  void populateCommandBuffer(VkCommandBuffer commandBuffer, int currentImage) {
    
    // Offscreen pass - always required
    // begin standard pass
    RP.begin(commandBuffer, currentImage);

    // Draw the background first at the far depth. Scene geometry then covers it.
    Psky.bind(commandBuffer);
    DSsky.bind(commandBuffer, Psky, 0, currentImage);
    vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    
    SC.populateCommandBuffer(commandBuffer, 0, currentImage);
    
    RP.end(commandBuffer);
  }
  
  // Here is where you update the uniforms.
  // Very likely this will be where you will be writing the logic of your application.
  void updateUniformBuffer(uint32_t currentImage) {
    static bool debounce = false;
    static int curDebounce = 0;
    
    // handle the ESC key to exit the app
    if(glfwGetKey(window, GLFW_KEY_ESCAPE)) {
      glfwSetWindowShouldClose(window, GL_TRUE);
    }
    
    // moves the view
    float deltaT = GameLogic();

    // Remove camera translation before inverting the sky view-projection matrix.
    // This keeps celestial directions fixed while allowing the camera to rotate.
    const glm::mat4 projection = ViewPrj * glm::inverse(View);
    const glm::mat4 viewRotation = glm::mat4(glm::mat3(View));
    SkyUniformBufferObject skyUbo{};
    skyUbo.inverseViewProjection = glm::inverse(projection * viewRotation);
    skyUbo.resolution = glm::vec2(static_cast<float>(RP.width), static_cast<float>(RP.height));
    DSsky.map(currentImage, &skyUbo, 0);
    
    // defines the global parameters for the uniform
    static float lightRotationAngle = 0.0f; // Static variable to keep track of rotation
    lightRotationAngle += -0.5f * deltaT; // Increment rotation angle based on time
    
    const glm::mat4 lightView = glm::rotate(glm::mat4(1), glm::radians(lightRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * 
      glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3 lightDir =  glm::vec3(lightView * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    
    GlobalUniformBufferObject gubo{};
    
    gubo.lightDir = lightDir;
    gubo.lightColor = glm::vec4(0.55f, 0.68f, 1.0f, 1.0f) * 0.85f;
    gubo.eyePos = glm::vec3(glm::inverse(View)[3]);

    gubo.pointLightPos[0] = glm::vec4(79.0f, 1024.0f, 85.0f, 1.0f);
    gubo.pointLightPos[1] = glm::vec4(125.0f, 1040.0f, 75.0f, 1.0f);
    gubo.pointLightPos[2] = glm::vec4(79.0f, 1056.0f, 45.0f, 1.0f);
    gubo.pointLightPos[3] = glm::vec4(125.0f, 1072.0f, 25.0f, 1.0f);
    for (int i = 0; i < 4; ++i) {
      gubo.pointLightColor[i] = glm::vec4(4.0f, 1.8f, 0.6f, 1.0f);
    }
    gubo.pointLightPos[4] = glm::vec4(-2.5f, 4.5f, 18.0f, 1.0f);
    gubo.pointLightPos[5] = glm::vec4( 2.5f, 4.5f, 18.0f, 1.0f);
    gubo.pointLightColor[4] = glm::vec4(2.8f, 1.1f, 0.35f, 1.0f);
    gubo.pointLightColor[5] = glm::vec4(2.8f, 1.1f, 0.35f, 1.0f);
    
    DSglobal.map(currentImage, &gubo, 0);
    
    // defines the local parameters for the uniforms
    UniformBufferObject ubo{};		
    const int wallModelId = SC.MeshIds["wall"];
    const int stairModelId = SC.MeshIds["stair"];
    const int floorModelId = SC.MeshIds["floor"];
    const int exteriorPathModelId = SC.MeshIds["exterior_path"];
    const int exteriorStoneModelId = SC.MeshIds["exterior_stone_block"];
    
    int instanceId;
    // character
    for(instanceId = 0; instanceId < SC.TI[0].InstanceCount; instanceId++) {
      ubo.mMat = SC.TI[0].I[instanceId].Wm;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      float worldDeterminant = glm::determinant(ubo.mMat);
      ubo.normalMat = glm::abs(worldDeterminant) > 0.000001f
          ? glm::transpose(glm::inverse(ubo.mMat))
          : glm::mat4(1.0f);
      bool tiledSurface = SC.TI[0].I[instanceId].Mid == wallModelId ||
                          SC.TI[0].I[instanceId].Mid == stairModelId ||
                          SC.TI[0].I[instanceId].Mid == floorModelId ||
                          SC.TI[0].I[instanceId].Mid == exteriorPathModelId ||
                          SC.TI[0].I[instanceId].Mid == exteriorStoneModelId;
      const float albedoScale = SC.TI[0].I[instanceId].Mid == exteriorPathModelId
          ? 0.62f
          : 1.0f;
      ubo.surfaceParams = glm::vec4(0.12f, tiledSurface ? 1.0f : 0.0f, albedoScale, 0.0f);
      
      // DS[1] = Pchar pass (main render): set0=DSLglobal, set1=DSLlocal
      SC.TI[0].I[instanceId].DS[0][0]->map(currentImage, &gubo, 0); // global (light/camera)
      SC.TI[0].I[instanceId].DS[0][1]->map(currentImage, &ubo, 0); // camera MVPs
    }
    
    // updates the FPS
    static float elapsedT = 0.0f;
    static int countedFrames = 0;
    
    countedFrames++;
    elapsedT += deltaT;
    if(elapsedT > 1.0f) {
      float Fps = (float)countedFrames / elapsedT;
      
      std::ostringstream oss;
      oss << "FPS: " << Fps << "\n";std::vector<float>{0.0f, 0.0f, 0.0f};
      oss << "Player X: " << camPos.x << "\n";
      oss << "Player Y: " << camPos.y << "\n";
      oss << "Player Z: " << camPos.z << "\n";
      if (gameState == GameState::Playing && hasPotion) {
        oss << "Potion acquired - Ghosts remaining: " << remainingGhosts << "\n";
      } else if (gameState == GameState::Playing) {
        oss << "Find the potion\n";
      }
      
      txt.print(1.0f, 1.0f, oss.str(), 1, "CO", false, false, true,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
      
      elapsedT = 0.0f;
      countedFrames = 0;
    }
    
    txt.updateCommandBuffer();
  }

  void resetGame() {
    // 1. Reset Camera / Player state
    camPos = glm::vec3(0.0f, 2.5f, 80.0f);
    camYaw = glm::radians(180.0f);
    camPitch = glm::radians(-10.0f);
    velocity_y = 0.0f;
    isGrounded = false;

    // 2. Reset Rock physics and states
    rock1Pos = glm::vec3(115.0f, 1080.0f, 10.0f);
    rock2Pos = glm::vec3(90.0f, 1080.0f, 10.0f);
    rock1VelY = 0.0f;
    rock2VelY = 0.0f;
    rock1Grounded = false;
    rock2Grounded = false;
    rock1Angle = 0.0f;
    rock2Angle = 0.0f;
    rockStart = false;
    rockStop = false;
    gameState = GameState::Playing;
    hasPotion = false;
    potionAvailable = true;
    fireWasPressed = false;
    remainingGhosts = 0;
    spawnedGhosts = 0;
    txt.removeText(2);

    // 3. Reset Active Ghosts vector
    std::fill(activeGhosts.begin(), activeGhosts.end(), false);
    std::fill(brokenGraves.begin(), brokenGraves.end(), false);
    std::fill(collisionDisabled.begin(), collisionDisabled.end(), false);
    ghostDirections = initialGhostDirections;

    // 4. Reset Ghost Directions if you added direction tracking
    // std::fill(ghostDirections.begin(), ghostDirections.end(), 1.0f);

    // 5. Restore Grave, Broken Grave, and Ghost World Matrices
    for (int i = 0; i < graveIdCounter; ++i) {
      std::string graveId  = "grave_auto_" + std::to_string(i);
      std::string brokenId = "broken_grave_auto_" + std::to_string(i);
      std::string ghostId  = "ghost_auto_" + std::to_string(i);

      if (instanceIndexMap.count(graveId) &&
        instanceIndexMap.count(brokenId) &&
        instanceIndexMap.count(ghostId)) {

        int mainIdx   = instanceIndexMap[graveId];
      int brokenIdx = instanceIndexMap[brokenId];
      int ghostIdx  = instanceIndexMap[ghostId];

      Instance &mainGrave   = SC.TI[0].I[mainIdx];
      Instance &brokenGrave = SC.TI[0].I[brokenIdx];
      Instance &ghost       = SC.TI[0].I[ghostIdx];

      mainGrave.Wm = initialGraveTransforms[i];
      brokenGrave.Wm = initialBrokenGraveTransforms[i];
      ghost.Wm = initialGhostTransforms[i];
        }
    }

    if (potionTransformSaved && instanceIndexMap.count("hidden_room_potion")) {
      SC.TI[0].I[instanceIndexMap["hidden_room_potion"]].Wm = initialPotionTransform;
    }

    // 6. Reset Rock Instance Matrices in Scene
    if (instanceIndexMap.count("rock1") && instanceIndexMap.count("rock2")) {
      int idx1 = instanceIndexMap["rock1"];
      int idx2 = instanceIndexMap["rock2"];
      glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));

      SC.TI[0].I[idx1].Wm = glm::translate(glm::mat4(1.0f), rock1Pos) * scaleMat;
      SC.TI[0].I[idx2].Wm = glm::translate(glm::mat4(1.0f), rock2Pos) * scaleMat;
    }
  }

  bool checkSceneCollision(Collider &playerCol) {
    for (int t = 0; t < SC.TechniqueInstanceCount; t++) {
      for (int i = 0; i < SC.TI[t].InstanceCount; i++) {
	Instance &inst = SC.TI[t].I[i];

	if (t == 0 && i < collisionDisabled.size() && collisionDisabled[i]) {
	  continue;
	}

	if (inst.C != nullptr) {
	  if (playerCol.collidesWith(*(inst.C))) {
	    return true;
	  }
	}
      }
    }
    return false;
  }

  bool checkStairCollision(Collider &objectCol) {
    for (int t = 0; t < SC.TechniqueInstanceCount; t++) {
      for (int i = 0; i < SC.TI[t].InstanceCount; i++) {
        Instance &inst = SC.TI[t].I[i];

        if (inst.C != nullptr) {
          // Find the instance ID using our map
          std::string instId = "";
          for (const auto& [id, index] : instanceIndexMap) {
            if (index == i) {
              instId = id;
              break;
            }
          }

          // Only perform collision check if the object is a stair step
          if (instId.find("stair") == 0) {
            if (objectCol.collidesWith(*(inst.C))) {
              return true;
            }
          }
        }
      }
    }
    return false;
  }

  void checkGraveCollisions(Collider &rockCol) {
    for (int i = 0; i < graveIdCounter; ++i) {
      // Skip graves that have already been broken.
      if (i < brokenGraves.size() && brokenGraves[i]) continue;

      std::string graveId  = "grave_auto_" + std::to_string(i);
      std::string brokenId = "broken_grave_auto_" + std::to_string(i);
      std::string ghostId  = "ghost_auto_" + std::to_string(i);

      if (instanceIndexMap.count(graveId)) {
        int mainIdx   = instanceIndexMap[graveId];
        int brokenIdx = instanceIndexMap[brokenId];
        int ghostIdx  = instanceIndexMap[ghostId];

        Instance &mainGrave = SC.TI[0].I[mainIdx];

        if (mainGrave.C != nullptr && rockCol.collidesWith(*(mainGrave.C))) {
          brokenGraves[i] = true;
          activeGhosts[i] = true;
          remainingGhosts++;
          spawnedGhosts++;

          // Hide original grave
          mainGrave.Wm = glm::scale(mainGrave.Wm, glm::vec3(0.0f));
          collisionDisabled[mainIdx] = true;

          // Reveal broken grave & ghost
          Instance &brokenGrave = SC.TI[0].I[brokenIdx];
          brokenGrave.Wm = glm::scale(initialBrokenGraveTransforms[i], glm::vec3(1500.0f));

          Instance &ghost = SC.TI[0].I[ghostIdx];
          ghost.Wm = glm::scale(initialGhostTransforms[i], glm::vec3(1500.0f));
        }
      }
    }
  }

  void rockLogic(float deltaT) {
    if (rockStart) {
      float rock1Speed = 10.0f; // Constant forward speed down the stairs
      float rock2Speed = 9.0f; // Constant forward speed down the stairs
      float rockRadius = 12.0f; // Adjust based on your rock scale

      // --- ROCK 1 UPDATE ---
      if (!rock1Grounded) {
        rock1VelY += gravity * deltaT;
      }
      rock1Pos.z += rock1Speed * deltaT; // Move forward along Z
      rock1Pos.y += rock1VelY * deltaT;  // Apply vertical gravity

      // Simple Y Collision check for Rock 1 (using AABB/Sphere test)
      Collider rockCol1;
      rockCol1.initSphere(rock1Pos.x, rock1Pos.y, rock1Pos.z, rockRadius);
      rockCol1.setWorldMatrix(glm::mat4(1.0f));

      // Check collision (make sure to ignore the rock's own index if needed,
      // or rely on environment collisions like stairs/walls)
      if (checkStairCollision(rockCol1)) {
        rock1Pos.y -= rock1VelY * deltaT; // Revert step if collided
        rock1VelY = 0.0f;
        rock1Grounded = true;
      } else {
        rock1Grounded = false;
      }

      checkGraveCollisions(rockCol1);

      if (instanceIndexMap.find("wall_B") != instanceIndexMap.end())
      {
        if (rockCol1.collidesWith(*((SC.TI[0].I[instanceIndexMap["wall_B"]].C))))
        {
          rock1Pos.z -= rock1Speed * deltaT;
        }
        else
        {
          rock1Angle += deltaT * 0.2f * rock1Speed;
          rock1Angle = std::fmod(rock1Angle, 2.0f * glm::pi<float>());
        }
      }

      // --- ROCK 2 UPDATE (Symmetrical) ---
      if (!rock2Grounded) {
        rock2VelY += gravity * deltaT;
      }
      rock2Pos.z += rock2Speed * deltaT;
      rock2Pos.y += rock2VelY * deltaT;

      Collider rockCol2;
      rockCol2.initSphere(rock2Pos.x, rock2Pos.y, rock2Pos.z, rockRadius);
      rockCol2.setWorldMatrix(glm::mat4(1.0f));

      if (checkStairCollision(rockCol2)) {
        rock2Pos.y -= rock2VelY * deltaT;
        rock2VelY = 0.0f;
        rock2Grounded = true;
      } else {
        rock2Grounded = false;
      }

      if (instanceIndexMap.find("wall_B") != instanceIndexMap.end())
      {
        if (rockCol2.collidesWith(*((SC.TI[0].I[instanceIndexMap["wall_B"]].C))))
        {
          rock2Pos.z -= rock2Speed * deltaT;
        }
        else
        {
          rock2Angle += deltaT * 0.2f * rock2Speed;
          rock2Angle = std::fmod(rock2Angle, 2.0f * glm::pi<float>());
        }
      }

      Collider playerCol;
      playerCol.initSphere(camPos.x, camPos.y - playerHeight / 2.0f, camPos.z, playerHeight / 2.0f);

      if (rockCol2.collidesWith(playerCol) || rockCol1.collidesWith(playerCol))
      {
        resetGame();
        return;
      }

      checkGraveCollisions(rockCol2);


      // --- APPLY TO ENGINE INSTANCE MATRICES ---
      if (instanceIndexMap.find("rock1") != instanceIndexMap.end() &&
        instanceIndexMap.find("rock2") != instanceIndexMap.end()) {

        int idx1 = instanceIndexMap["rock1"];
      int idx2 = instanceIndexMap["rock2"];

      // Rebuild World Matrix with translation and continuous rolling rotation
      glm::mat4 rot1X = glm::rotate(glm::mat4(1.0f), rock1Angle, glm::vec3(1.0f, 0.0f, 0.0f));
      glm::mat4 rot2X = glm::rotate(glm::mat4(1.0f), rock2Angle, glm::vec3(1.0f, 0.0f, 0.0f));
      glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.37f, 0.37f, 0.37f));

      SC.TI[0].I[idx1].Wm = glm::translate(glm::mat4(1.0f), rock1Pos) * rot1X * scaleMat;
      SC.TI[0].I[idx2].Wm = glm::translate(glm::mat4(1.0f), rock2Pos) * rot2X * scaleMat;
        }
    }
  }

  void ghostLogic(float deltaT) {
    const float ghostRadius = 0.4f;
    const float ghostPosMax = 126.5f;
    const float ghostPosMin = 77.5f;

    for (int i = 0; i < graveIdCounter; ++i) {
      if (i < activeGhosts.size() && activeGhosts[i]) {
        std::string ghostId = "ghost_auto_" + std::to_string(i);

        if (instanceIndexMap.count(ghostId)) {
          int ghostIdx = instanceIndexMap[ghostId];
          Instance &ghost = SC.TI[0].I[ghostIdx];

          Collider playerCol;
          playerCol.initSphere(camPos.x, camPos.y - playerHeight / 2.0f, camPos.z, playerHeight / 2.0f);

          glm::vec3 pos = glm::vec3(ghost.Wm[3]);
          Collider ghostCol;
          ghostCol.initSphere(pos.x, pos.y, pos.z, ghostRadius);

          if (ghostCol.collidesWith(playerCol))
          {
            resetGame();
            return;
          }

          // 1. Check boundary conditions and flip direction/rotation
          if (ghostDirections[i] < 0.0f && pos.x <= ghostPosMin) {
            ghostDirections[i] = 1.0f; // Turn around to move right (+X)
          } else if (ghostDirections[i] > 0.0f && pos.x >= ghostPosMax) {
            ghostDirections[i] = -1.0f; // Turn around to move left (-X)
          }

          float targetYaw = (ghostDirections[i] < 0.0f) ? glm::radians(-90.0f) : glm::radians(90.0f);

          if (pos.x <= ghostPosMin + 5.0f)
          {
            if (ghostDirections[i] < 0.0f)
            {
              targetYaw = glm::radians(-90.0f + (ghostPosMin + 5.0f - pos.x) * 180.0f / 5.0f);
            }
            pos.x += ghostDirections[i] * ghostSpeeds[i] * (pos.x - ghostPosMin + 0.1f) / 5.1f * deltaT;
          }
          else if (pos.x >= ghostPosMax - 5.0f)
          {
            if (ghostDirections[i] > 0.0f)
            {
              targetYaw = glm::radians(90.0f - (pos.x - ghostPosMax + 5.0f) * 180.0f / 5.0f);
            }
            pos.x += ghostDirections[i] * ghostSpeeds[i] * (ghostPosMax + 0.1f - pos.x) / 5.1f * deltaT;
          }
          else
          {
            pos.x += ghostDirections[i] * ghostSpeeds[i] * deltaT;
          }

          // 4. Reconstruct World Matrix with translation, rotation, and 1.5 scale
          glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), targetYaw, glm::vec3(0.0f, 1.0f, 0.0f));
          glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.5f));

          ghost.Wm = glm::translate(glm::mat4(1.0f), pos) * rotMat * scaleMat;
        }
      }
    }
  }

  void handleInteraction(bool fire, const glm::vec3 &forward) {
    bool firePressed = fire && !fireWasPressed;
    fireWasPressed = fire;
    if (!firePressed) {
      return;
    }

    if (potionAvailable && instanceIndexMap.count("hidden_room_potion")) {
      int potionIdx = instanceIndexMap["hidden_room_potion"];
      glm::vec3 potionPos = glm::vec3(SC.TI[0].I[potionIdx].Wm[3]);
      if (glm::length(potionPos - camPos) <= 4.0f) {
        hasPotion = true;
        potionAvailable = false;
        SC.TI[0].I[potionIdx].Wm = glm::scale(initialPotionTransform, glm::vec3(0.0f));
        collisionDisabled[potionIdx] = true;
        return;
      }
    }

    if (!hasPotion) {
      return;
    }

    int targetGhost = -1;
    float closestDistance = 5.0f;
    glm::vec3 viewDirection = glm::normalize(forward);

    for (int i = 0; i < graveIdCounter; ++i) {
      if (!activeGhosts[i]) {
        continue;
      }

      int ghostIdx = instanceIndexMap["ghost_auto_" + std::to_string(i)];
      glm::vec3 ghostPos = glm::vec3(SC.TI[0].I[ghostIdx].Wm[3]);
      glm::vec3 toGhost = ghostPos - camPos;
      float distance = glm::length(toGhost);

      if (distance > 0.0f && distance <= closestDistance &&
          glm::dot(glm::normalize(toGhost), viewDirection) >= 0.8f) {
        closestDistance = distance;
        targetGhost = i;
      }
    }

    if (targetGhost >= 0) {
      int ghostIdx = instanceIndexMap["ghost_auto_" + std::to_string(targetGhost)];
      SC.TI[0].I[ghostIdx].Wm =
          glm::scale(initialGhostTransforms[targetGhost], glm::vec3(0.0f));
      activeGhosts[targetGhost] = false;
      remainingGhosts--;
    }
  }

  void checkEndCondition() {
    const bool reachedTop = camPos.y >= 1065.0f && camPos.z <= 0.0f;
    if (!reachedTop) {
      return;
    }

    // Without the potion, surviving the ghosts and reaching the top is enough.
    // After drinking the potion, every spawned ghost must have been defeated.
    if (!hasPotion || remainingGhosts == 0) {
      gameState = GameState::Won;
    } else {
      gameState = GameState::Lost;
    }

    std::string result = "YOU WIN";
    if (gameState == GameState::Won)
    {
      if (hasPotion)
      {
        result = "YOU WIN \n BUT AT WHAT COST?";
      }
    }
    else
    {
      result = "YOU LOSE \n GHOSTS REMAINING: " + std::to_string(remainingGhosts);
    }
    glm::vec4 resultColor = gameState == GameState::Won
        ? glm::vec4(0.25f, 1.0f, 0.35f, 1.0f)
        : glm::vec4(1.0f, 0.2f, 0.2f, 1.0f);
    txt.print(0.0f, 0.0f,
              "====================\n" + result +
              "\n\n[ REPLAY ]\nPress R \n====================",
              2, "SS", false, true, false,
              TAL_CENTER, TRH_CENTER, TRV_MIDDLE,
              resultColor,
              glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
              glm::vec4(0.0f, 0.0f, 0.0f, 0.8f),
              2.0f, 2.0f);
  }
  
  float GameLogic() {
    const float FOVy = glm::radians(75.0f);
    const float nearPlane = 0.1f;
    const float farPlane = 200.f;
    
    float deltaT;
    glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
    bool fire = false;
    getSixAxis(deltaT, m, r, fire);

    bool replayPressed = (m.y > 0);

    if (gameState != GameState::Playing) {
      if (replayPressed && !replayWasPressed) {
        resetGame();
      }
      replayWasPressed = replayPressed;
      fireWasPressed = (m.y > 0);
      return deltaT;
    }
    replayWasPressed = replayPressed;
    
    const float moveSpeed = 8.0f;   
    const float rotateSpeed = 25.5f;

    std::vector<float> jumpCurve= { 0.00f, 0.35f, 0.75f, 1.10f, 1.35f, 1.45f, 1.50f, 1.55f, 1.50f, 1.45f, 1.35f, 1.10f, 0.75f, 0.35f, 0.00f };
   
    static bool isCrouching = false;
    static size_t currentFrame = 0;
    static float frameAccumulator = 0.0f;
    const float timePerFrame = 0.03f;

    if (!isCrouching && m.y < 0) {
	  isCrouching = true;
      currentFrame = 0;
      frameAccumulator = 0.0f;
    }

    float heightOffset = 0.0f;
    if (isCrouching) {
      frameAccumulator += deltaT;
      if (frameAccumulator >= timePerFrame) {
	if ((currentFrame < jumpCurve.size()/2 || m.y > -0.1f) &&
	    currentFrame + 1 < jumpCurve.size()) {
	  currentFrame++;
	}
	frameAccumulator = 0.0f;
      }
      
      heightOffset = -jumpCurve[currentFrame];
	
      if (currentFrame == jumpCurve.size() - 1){
        isCrouching = false;
      }
    }
    
    camYaw   += r.y * rotateSpeed * deltaT;
    camPitch -= r.x * rotateSpeed * deltaT;
    
    camPitch = glm::clamp(camPitch, glm::radians(-85.0f), glm::radians(85.0f));
    
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), camYaw, glm::vec3(0.0f, 1.0f, 0.0f)) *
    glm::rotate(glm::mat4(1.0f), camPitch, glm::vec3(1.0f, 0.0f, 0.0f));
    
    glm::vec3 forward = glm::vec3(rotationMatrix * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f));
    glm::vec3 right   = glm::vec3(rotationMatrix * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    glm::vec3 up      = glm::vec3(0.0f, 1.0f, 0.0f);
    
    
    glm::vec3 walkForward = glm::normalize(glm::vec3(forward.x, 0.0f, forward.z));
    glm::vec3 walkRight   = glm::normalize(glm::vec3(right.x, 0.0f, right.z));

    if (camPos.x < 2.0f && camPos.x > -2.0f &&  camPos.z <17.5 &&  camPos.z >17)
    {
      camPos = glm::vec3(102.0f, 1014.0f, 92.0f);
    }
    if (camPos.y > 1000.0f && camPos.z > 80.0f && camPos.z < 82.0f)
    {
      if (forward.z < 0)
      {
        camPos = glm::vec3(camPos.x, camPos.y - 8.0f, 92.0f);
      }
      else if (rockStart == false)
      {
        rockStart = true;
        if (instanceIndexMap.find("rock1") != instanceIndexMap.end() &&
          instanceIndexMap.find("rock2") != instanceIndexMap.end()) {

        int idx1 = instanceIndexMap["rock1"];
        int idx2 = instanceIndexMap["rock2"];

        // Rebuild World Matrix with proper scale
        glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.37f, 0.37f, 0.37f));

        SC.TI[0].I[idx1].Wm = glm::translate(glm::mat4(1.0f), rock1Pos) * scaleMat;
        SC.TI[0].I[idx2].Wm = glm::translate(glm::mat4(1.0f), rock2Pos) * scaleMat;
      }
    }
    }

    if (isGrounded && fire) {
      velocity_y = jumpImpulse;
      isGrounded = false;
    }

    float playerRadius = 0.4f;

    if (!isGrounded) {
      velocity_y += gravity * deltaT;
    }

    // Optional: Terminal velocity cap to prevent falling through floors at low FPS
    if (velocity_y < -30.0f) velocity_y = -30.0f;

    glm::vec3 intendedMove = (-walkForward * m.z * moveSpeed * deltaT) +
                             (-walkRight   * m.x * moveSpeed * deltaT) +
                             (up * velocity_y * deltaT);

    // 3. Assume in air until collision check proves otherwise
    isGrounded = false;
    // 1. Check Y
    glm::vec3 testPosY = camPos + glm::vec3(0.0f, intendedMove.y, 0.0f);
    Collider playerColY;
    playerColY.initAABB(testPosY.x - playerRadius, testPosY.y - playerHeight, testPosY.z - playerRadius, testPosY.x + playerRadius, testPosY.y, testPosY.z + playerRadius);
    playerColY.setWorldMatrix(glm::mat4(1.0f));
    if (!checkSceneCollision(playerColY) && testPosY.y > playerHeight) {
      camPos.y = testPosY.y;
    }
    else
    {
      if (velocity_y < 0.0f)
      {
        isGrounded = true;
      }
      velocity_y = 0;
    }
 
    // 1. Check X
    glm::vec3 testPosX = camPos + glm::vec3(intendedMove.x, 0.0f, 0.0f);
    Collider playerColX;
    playerColX.initAABB(testPosX.x - playerRadius, testPosX.y - playerHeight, testPosX.z - playerRadius, testPosX.x + playerRadius,
                        testPosX.y, testPosX.z + playerRadius);
    playerColX.setWorldMatrix(glm::mat4(1.0f));
    if (!checkSceneCollision(playerColX)) {
      camPos.x = testPosX.x;
    }
 
    // 2. Check Z (Keeping Y stable unless you add gravity later!)
    glm::vec3 testPosZ = camPos + glm::vec3(0.0f, 0.0f, intendedMove.z);
    Collider playerColZ;
    playerColZ.initAABB(testPosZ.x - playerRadius, testPosZ.y - playerHeight, testPosZ.z - playerRadius, testPosZ.x + playerRadius,
                        testPosZ.y, testPosZ.z + playerRadius);
    //playerColZ.initSphere(testPosZ.x, testPosZ.y, testPosZ.z, playerRadius);
    playerColZ.setWorldMatrix(glm::mat4(1.0f));
    if (!checkSceneCollision(playerColZ)) {
      camPos.z = testPosZ.z;
    }
        
    glm::vec3 renderedCamPos = camPos;
    renderedCamPos.y += heightOffset;

    glm::mat4 Prj = glm::perspective(FOVy, Ar, nearPlane, farPlane);
    Prj[1][1] *= -1;
    
    View = glm::lookAt(renderedCamPos, renderedCamPos + forward, up);
    
    ViewPrj = Prj * View;


    rockLogic(deltaT);

    ghostLogic(deltaT);

    handleInteraction(m.y > 0, forward);

    checkEndCondition();
    
    return deltaT;
  }
};


// This is the main: probably you do not need to touch this!
int main() {
  Skeleton26ReplaceName app;
  
  try {
    app.run(false);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  
  return EXIT_SUCCESS;
}
