// TAAHIS IS THE FILE YOU MUST START FROM!A

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
};

struct GlobalUniformBufferObject {
  alignas(16) glm::vec3 lightDir;
  alignas(16) glm::vec4 lightColor;
  alignas(16) glm::vec3 eyePos;
};

struct Vertex {
  glm::vec3 pos;
  glm::vec2 UV;
};

// MAIN !

class Skeleton26ReplaceName : public BaseProject {
protected:
  // Here you list all the Vulkan objects you need:
  
  // Descriptor Layouts [what will be passed to the shaders]
  DescriptorSetLayout DSLlocal, DSLglobal;
  
  // Vertex formants, Pipelines [Shader couples] and Render passes
  VertexDescriptor VD;
  RenderPass RP;
  Pipeline P;
  
  // Models, textures and Descriptors (values assigned to the uniforms)
  DescriptorSet DSglobal;
  
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
  glm::vec3 camPos = glm::vec3(20.0f, 2.5f, 20.0f); // Initial spawn position
  float camYaw = 0.0f;                             // Horizontal look angle
  float camPitch = 0.0f;                           // Vertical look angle
  // Rock physics state
  glm::vec3 rock1Pos = glm::vec3(115.0f, 1080.0f, 10.0f);
  glm::vec3 rock2Pos = glm::vec3(90.0f, 1080.0f, 10.0f);
  float rock1VelY = 0.0f;
  float rock2VelY = 0.0f;
  bool rock1Grounded = false;
  bool rock2Grounded = false;
  const float ghostRadius = 0.7f;
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
  bool hasPotion = false;
  std::vector<float> ghostDirections;
  std::vector<bool> activeGhosts;

  // Initial States
  struct InitialInstanceState {
    glm::mat4 Wm;
    Collider* C;
  };
  std::vector<InitialInstanceState> initialInstances;
  std::vector<float> initialGhostDirections;


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
	{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, sizeof(UniformBufferObject), 1},
	{1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 0, 1}
      });
    DSLglobal.init(this, {
	// this array contains the binding:
	// first  element : the binding number
	// second element : the type of element (buffer or texture)
	// third  element : the pipeline stage where it will be used
	{0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS, sizeof(GlobalUniformBufferObject), 1}
      });
    VD.init(this, {
	{0, sizeof(Vertex), VK_VERTEX_INPUT_RATE_VERTEX}
      }, {
	{0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, pos),
	 sizeof(glm::vec3), POSITION},
	{0, 1, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, UV),
	 sizeof(glm::vec2), UV}
      });
    
    // initializes the render passes
    RP.init(this);
    // sets the blue sky
    RP.properties[0].clearValue = {0.0f,0.3f,0.33f,0.33f};

    
    // Pipelines [Shader couples]
    // The last array, is a vector of pointer to the layouts of the sets that will
    // be used in this pipeline. The first element will be set 0, and so on..
    
    P.init(this, &VD, "shaders/toChangeSimplePos.vert.spv",
	   "shaders/toChangeBlinnFromPos.frag.spv",
	   {&DSLglobal, &DSLlocal});
    
    
    // sets the size of the Descriptor Set Pool (it MUST be done before loading the scene)
    DPSZs.uniformBlocksInPool = 2;
    DPSZs.texturesInPool = 1;
    DPSZs.setsInPool = 2;
    
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
    std::uniform_int_distribution<> numGravesDist(2, 3);
    std::uniform_real_distribution<float> xPosDist0(80.0f, 94.0f);
    std::uniform_real_distribution<float> xPosDist2(95.0f, 109.0f);
    std::uniform_real_distribution<float> xPosDist1(110.0f, 124.0f);
    std::uniform_real_distribution<float> yRotDist(-45.0f, 45.0f);

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
      int graveID = graveIdCounter++;

      int gravesOnThisStep = numGravesDist(gen);

      for (int g = 0; g < gravesOnThisStep; ++g) {
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

    initialGhostDirections = ghostDirections;

    initialInstances.resize(SC.TI[0].InstanceCount);
    for (int i = 0; i < SC.TI[0].InstanceCount; ++i) {
      initialInstances[i].Wm = SC.TI[0].I[i].Wm;
      initialInstances[i].C  = SC.TI[0].I[i].C;
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
    
    DSglobal.init(this, &DSLglobal, {});
    
    // Here you define the data set
    // If the scene has textures coming from a render pass, the corresponding element of the technique must be
    // updated before calling SC.pipelinesAndDescriptorSetsInit();
    
    SC.pipelinesAndDescriptorSetsInit();
    txt.pipelinesAndDescriptorSetsInit();
  }
  
  // Here you destroy your pipelines and Descriptor Sets!
  void pipelinesAndDescriptorSetsCleanup() {
    P.cleanup();
    
    RP.cleanup();
    
    DSglobal.cleanup();
    
    SC.pipelinesAndDescriptorSetsCleanup();
    txt.pipelinesAndDescriptorSetsCleanup();
  }
  
  // Here you destroy all the Models, Texture and Desc. Set Layouts you created!
  // You also have to destroy the pipelines
  void localCleanup() {
    DSLlocal.cleanup();
    DSLglobal.cleanup();
    
    P.destroy();
    
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
    
    // defines the global parameters for the uniform
    static float lightRotationAngle = 0.0f; // Static variable to keep track of rotation
    lightRotationAngle += -0.5f * deltaT; // Increment rotation angle based on time
    
    const glm::mat4 lightView = glm::rotate(glm::mat4(1), glm::radians(lightRotationAngle), glm::vec3(0.0f, 1.0f, 0.0f)) * 
      glm::rotate(glm::mat4(1), glm::radians(-45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    const glm::vec3 lightDir =  glm::vec3(lightView * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f));
    
    GlobalUniformBufferObject gubo{};
    
    gubo.lightDir = lightDir;
    gubo.lightColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f)*5.0f;
    gubo.eyePos = glm::vec3(glm::inverse(View)[3]);
    
    DSglobal.map(currentImage, &gubo, 0);
    
    // defines the local parameters for the uniforms
    UniformBufferObject ubo{};		
    
    int instanceId;
    // character
    for(instanceId = 0; instanceId < SC.TI[0].InstanceCount; instanceId++) {
      ubo.mMat = SC.TI[0].I[instanceId].Wm;
      ubo.mvpMat = ViewPrj * ubo.mMat;
      
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
      
      txt.print(1.0f, 1.0f, oss.str(), 1, "CO", false, false, true,TAL_RIGHT,TRH_RIGHT,TRV_BOTTOM,{1.0f,0.0f,0.0f,1.0f},{0.8f,0.8f,0.0f,1.0f});
      
      elapsedT = 0.0f;
      countedFrames = 0;
    }
    
    txt.updateCommandBuffer();
  }

  struct RaycastHit {
    bool hit = false;
    float distance = 0.0f;
    std::string objectId = "";
    int index = -1;
  };

  RaycastHit getObjectInSight(glm::vec3 origin, glm::vec3 direction, float maxDistance) {
    RaycastHit result;
    const float stepSize = 0.2f; // Distance to move forward each check

    // Step forward along the view direction
    for (float dist = 0.0f; dist <= maxDistance; dist += stepSize) {
      glm::vec3 checkPos = origin + (direction * dist);

      // Create a tiny collider at the current point on the ray
      Collider rayPoint;
      rayPoint.initSphere(checkPos.x, checkPos.y, checkPos.z, 0.1f);
      rayPoint.setWorldMatrix(glm::mat4(1.0f));

      // Test this point against all objects in the scene
      for (int t = 0; t < SC.TechniqueInstanceCount; t++) {
        for (int i = 0; i < SC.TI[t].InstanceCount; i++) {
          Instance &inst = SC.TI[t].I[i];

          if (inst.C != nullptr && rayPoint.collidesWith(*(inst.C))) {
            result.hit = true;
            result.distance = dist;
            result.index = i;

            // Look up the string ID
            for (const auto& [id, mappedIdx] : instanceIndexMap) {
              if (mappedIdx == i) {
                result.objectId = id;
                break;
              }
            }
            return result; // Return immediately on the first hit
          }
        }
      }
      for (int i = 0; i < graveIdCounter; ++i) {
        if (i < activeGhosts.size() && activeGhosts[i]) {
          std::string ghostId = "ghost_auto_" + std::to_string(i);
          if (instanceIndexMap.count(ghostId)) {
            int ghostIdx = instanceIndexMap[ghostId];
            Instance &ghost = SC.TI[0].I[ghostIdx];

            glm::vec3 pos = glm::vec3(ghost.Wm[3]);
            Collider ghostCol;
            ghostCol.initSphere(pos.x, pos.y, pos.z, ghostRadius);

            if (ghostCol.collidesWith(rayPoint))
            {
              result.hit = true;
              result.distance = dist;
              result.index = ghostIdx;
              result.objectId = ghostId;
              return result;
            }
          }
        }
      }
    }
    return result;
  }

  void resetGame() {
    // 1. Reset Camera / Player state
    camPos = glm::vec3(20.0f, 2.5f, 20.0f);
    camYaw = 0.0f;
    camPitch = 0.0f;
    velocity_y = 0.0f;
    isGrounded = false;
    hasPotion = false;

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

    // 3. Reset Ghost tracking states
    std::fill(activeGhosts.begin(), activeGhosts.end(), false);
    ghostDirections = initialGhostDirections;

    // 4. Restore ALL Scene Instances (Matrices & Colliders) to starting state
    for (int i = 0; i < SC.TI[0].InstanceCount; ++i) {
      if (i < initialInstances.size()) {
        SC.TI[0].I[i].Wm = initialInstances[i].Wm;
        SC.TI[0].I[i].C  = initialInstances[i].C;
      }
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
      // Skip if this grave index was already broken
      if (i < activeGhosts.size() && activeGhosts[i]) continue;

      std::string graveId  = "grave_auto_" + std::to_string(i);
      std::string brokenId = "broken_grave_auto_" + std::to_string(i);
      std::string ghostId  = "ghost_auto_" + std::to_string(i);

      if (instanceIndexMap.count(graveId)) {
        int mainIdx   = instanceIndexMap[graveId];
        int brokenIdx = instanceIndexMap[brokenId];
        int ghostIdx  = instanceIndexMap[ghostId];

        Instance &mainGrave = SC.TI[0].I[mainIdx];

        if (mainGrave.C != nullptr && rockCol.collidesWith(*(mainGrave.C))) {
          // Mark active
          if (i < activeGhosts.size()) activeGhosts[i] = true;

          // Hide original grave
          mainGrave.Wm = glm::scale(mainGrave.Wm, glm::vec3(0.0f));
          // Nullify collider so player collision checks instantly ignore it
          mainGrave.C = nullptr;

          // Reveal broken grave & ghost
          Instance &brokenGrave = SC.TI[0].I[brokenIdx];
          brokenGrave.Wm = glm::scale(brokenGrave.Wm, glm::vec3(1500.0f));

          Instance &ghost = SC.TI[0].I[ghostIdx];
          ghost.Wm = glm::scale(ghost.Wm, glm::vec3(1500.0f));
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

      if (instanceIndexMap.find("wall_B") != instanceIndexMap.end())
      {
        if (rockCol1.collidesWith(*((SC.TI[0].I[instanceIndexMap["wall_B"]].C))))
        {
          rock1Pos.z -= rock1Speed * deltaT;
        }
        else
        {
          rock1Angle += deltaT * 0.2f * rock1Speed;
          rock1Angle = std::fmod(rock2Angle, 2.0f * glm::pi<float>());
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
      }

      rockCol2.initSphere(rock2Pos.x, rock2Pos.y, rock2Pos.z, rockRadius*1.2f);
      rockCol2.setWorldMatrix(glm::mat4(1.0f));
      rockCol1.initSphere(rock1Pos.x, rock1Pos.y, rock1Pos.z, rockRadius*1.2f);
      rockCol1.setWorldMatrix(glm::mat4(1.0f));

      checkGraveCollisions(rockCol1);

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
    const float ghostSpeed = 9.0f; // Movement speed units/sec
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
            pos.x += ghostDirections[i] * ghostSpeed * (pos.x - ghostPosMin + 0.1f) / 5.1f * deltaT;
          }
          else if (pos.x >= ghostPosMax - 5.0f)
          {
            if (ghostDirections[i] > 0.0f)
            {
              targetYaw = glm::radians(90.0f - (pos.x - ghostPosMax + 5.0f) * 180.0f / 5.0f);
            }
            pos.x += ghostDirections[i] * ghostSpeed * (ghostPosMax + 0.1f - pos.x) / 5.1f * deltaT;
          }
          else
          {
            pos.x += ghostDirections[i] * ghostSpeed * deltaT;
          }

          // 4. Reconstruct World Matrix with translation, rotation, and 1.5 scale
          glm::mat4 rotMat = glm::rotate(glm::mat4(1.0f), targetYaw, glm::vec3(0.0f, 1.0f, 0.0f));
          glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.5f));

          ghost.Wm = glm::translate(glm::mat4(1.0f), pos) * rotMat * scaleMat;
        }
      }
    }
  }
  
  float GameLogic() {
    const float FOVy = glm::radians(75.0f);
    const float nearPlane = 0.1f;
    const float farPlane = 200.f;
    
    float deltaT;
    glm::vec3 m = glm::vec3(0.0f), r = glm::vec3(0.0f);
    bool fire = false;
    getSixAxis(deltaT, m, r, fire);
    
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
	if (currentFrame < jumpCurve.size()/2 || m.y > -0.1f) {
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

    // 1. Calculate raycast (e.g., max 4.0 units away)
    float maxPickupDistance = 4.0f;
    RaycastHit sightHit = getObjectInSight(camPos, forward, maxPickupDistance);

    // 2. Handle interaction if an object is within reach
    if (sightHit.hit) {

      if (fire) {
        if (sightHit.objectId.find("hidden_room_potion") == 0) {
          Instance &item = SC.TI[0].I[sightHit.index];
          item.Wm = glm::scale(glm::mat4(1.0f), glm::vec3(0.0f));
          item.C = nullptr;
          hasPotion = true;
        }
        if (sightHit.objectId.find("ghost_auto_") == 0 && hasPotion == true) {
          Instance &item = SC.TI[0].I[sightHit.index];
          item.Wm = glm::scale(glm::mat4(1.0f), glm::vec3(0.0f));
          item.C = nullptr;
        }
      }
    }

    if (isGrounded && m.y > 0) {
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
