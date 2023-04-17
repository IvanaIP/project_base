#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include <learnopengl/model.h>

#include <iostream>

void framebuffer_size_callback(GLFWwindow *window, int width, int height);

void mouse_callback(GLFWwindow *window, double xpos, double ypos);

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset);

void processInput(GLFWwindow *window);

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods);

const unsigned int SCR_WIDTH = 1920;
const unsigned int SCR_HEIGHT = 1080;

float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;

static int isPostProcessingEnabled = 0;
static int isHDREnabled = 0;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float rectV[] =
{
	// Coords    // texCoords
	 1.0f, -1.0f,  1.0f, 0.0f,
	-1.0f, -1.0f,  0.0f, 0.0f,
	-1.0f,  1.0f,  0.0f, 1.0f,

	 1.0f,  1.0f,  1.0f, 1.0f,
	 1.0f, -1.0f,  1.0f, 0.0f,
	-1.0f,  1.0f,  0.0f, 1.0f
};

struct PointLight {
    glm::vec3 position;
    glm::vec3 ambient;
    glm::vec3 diffuse;
    glm::vec3 specular;

    float constant;
    float linear;
    float quadratic;
};



struct ProgramState {
    glm::vec3 clearColor = glm::vec3(0);
    bool ImGuiEnabled = false;
    Camera camera;
    bool CameraMouseMovementUpdateEnabled = true;
    glm::vec3 backpackPosition = glm::vec3(0.0f);
    float backpackScale = 1.0f;
    PointLight pointLight;
    ProgramState()
            : camera(glm::vec3(0.0f, 0.0f, 3.0f)) {}

    void SaveToFile(std::string filename);

    void LoadFromFile(std::string filename);
};

void ProgramState::SaveToFile(std::string filename) {
    std::ofstream out(filename);
    out << clearColor.r << '\n'
        << clearColor.g << '\n'
        << clearColor.b << '\n'
        << ImGuiEnabled << '\n'
        << camera.Position.x << '\n'
        << camera.Position.y << '\n'
        << camera.Position.z << '\n'
        << camera.Front.x << '\n'
        << camera.Front.y << '\n'
        << camera.Front.z << '\n';
}

void ProgramState::LoadFromFile(std::string filename) {
    std::ifstream in(filename);
    if (in) {
        in >> clearColor.r
           >> clearColor.g
           >> clearColor.b
           >> ImGuiEnabled
           >> camera.Position.x
           >> camera.Position.y
           >> camera.Position.z
           >> camera.Front.x
           >> camera.Front.y
           >> camera.Front.z;
    }
}

ProgramState *programState;

void DrawImGui(ProgramState *programState);


//////////////////////////////////////////////////
//                                              //
//                     Main                     //
//                                              //
//////////////////////////////////////////////////
int main() {
    float gamma = 2.2f;
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL) {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    programState = new ProgramState;
    programState->LoadFromFile("resources/program_state.txt");
    if (programState->ImGuiEnabled) {
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    (void) io;

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    //////////////////////////////////////////////////
    //                                              //
    //            Inicijalizacija stanja            //
    //                                              //
    //////////////////////////////////////////////////
    glfwSwapInterval(0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    //////////////////////////////////////////////////
    //                                              //
    //                   Shaderi                    //
    //                                              //
    //////////////////////////////////////////////////
    Shader framebufferShader("resources/shaders/framebuffer.vs", "resources/shaders/framebuffer.fs");
    Shader grassShader("resources/shaders/grass.vs", "resources/shaders/grass.fs");
    Shader carLineShader("resources/shaders/carline.vs", "resources/shaders/carline.fs");
    Shader carShader("resources/shaders/default.vs", "resources/shaders/default.fs");
    Shader villaShader("resources/shaders/villa.vs", "resources/shaders/villa.fs");
    Shader clockShader("resources/shaders/clock.vs", "resources/shaders/clock.fs");
    Shader floorShader("resources/shaders/default.vs", "resources/shaders/default.fs");

    Model villaModel("resources/objects/futuristic_app/Futuristic\ Apartment.obj");
    Model carModel("resources/objects/car/car.obj");
    Model clockModel("resources/objects/clockwork/clock.obj");
    Model floorModel("resources/objects/floor/scene.gltf");
    Model grassModel("resources/objects/grass/scene.gltf");

	framebufferShader.use();
    framebufferShader.setInt("screenTexture", 0);
    glUniform1f(glGetUniformLocation(framebufferShader.ID, "gamma"), gamma);
    
    villaModel.SetShaderTextureNamePrefix("material.");
    carModel.SetShaderTextureNamePrefix("material.");
    clockModel.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 59.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(55.0, 104.0, 455.0);
    pointLight.constant = 1.0f;
    pointLight.linear = 0.0004f;
    pointLight.quadratic = 0.00038f;

    // FRAMEBUFFER
    unsigned int rectVAO, rectVBO;
	glGenVertexArrays(1, &rectVAO);
	glGenBuffers(1, &rectVBO);
	glBindVertexArray(rectVAO);
	glBindBuffer(GL_ARRAY_BUFFER, rectVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(rectV), &rectV, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));


	unsigned int FBO;
	glGenFramebuffers(1, &FBO);
	glBindFramebuffer(GL_FRAMEBUFFER, FBO);

	unsigned int framebufferTexture;
	glGenTextures(1, &framebufferTexture);
	glBindTexture(GL_TEXTURE_2D, framebufferTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, framebufferTexture, 0);

	unsigned int RBO;
	glGenRenderbuffers(1, &RBO);
	glBindRenderbuffer(GL_RENDERBUFFER, RBO);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, SCR_WIDTH, SCR_HEIGHT);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO);

	auto fboStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (fboStatus != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "Framebuffer error: " << fboStatus << std::endl;


    //////////////////////////////////////////////////
    //                                              //
    //             Petlja renderovanja              //
    //                                              //
    //////////////////////////////////////////////////
    while (!glfwWindowShouldClose(window)) {

        float currFrame = glfwGetTime();
        deltaTime = currFrame - lastFrame;
        lastFrame = currFrame;
        processInput(window);

        //////////////////////////////////////////////////
        //                                              //
        //                   Ciscenje                   //
        //                                              //
        //////////////////////////////////////////////////
        if (isPostProcessingEnabled) {
            if (isHDREnabled) {
                framebufferShader.use();
                framebufferShader.setBool("HDR", true);
            } else {
                framebufferShader.use();
                framebufferShader.setBool("HDR", false);
            }
            glBindFramebuffer(GL_FRAMEBUFFER, FBO);
        }
        glClearColor(pow(programState->clearColor.r,gamma), pow(programState->clearColor.g,gamma), pow(programState->clearColor.b,gamma), 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        // pozicija svetla
        pointLight.position = glm::vec3(150.0 * cos(currFrame), 120 + 100.0f * abs(cos(currFrame)), 150* sin(currFrame/10));


        ////////////////////////////////////////////////////
        //                                                //
        //              Crtanje modela auta               //
        //                                                //
        ////////////////////////////////////////////////////

        carShader.use();
        carShader.setVec3("pointLight.position", pointLight.position);
        carShader.setVec3("pointLight.ambient", pointLight.ambient);
        carShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        carShader.setVec3("pointLight.specular", pointLight.specular);
        carShader.setFloat("pointLight.constant", pointLight.constant);
        carShader.setFloat("pointLight.linear", pointLight.linear);
        carShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        carShader.setVec3("viewPosition", programState->camera.Position);
        carShader.setFloat("material.shininess", 32.0f);
        glm::mat4 car_projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 car_view = programState->camera.GetViewMatrix();
        carShader.setMat4("projection", car_projection);
        carShader.setMat4("view", car_view);
        glm::mat4 car_model = glm::mat4(1.0f);
        car_model = glm::translate(car_model, programState->backpackPosition + glm::vec3(0, 0, 45));
        car_model = glm::scale(car_model, glm::vec3(programState->backpackScale));
        carShader.setMat4("model", car_model);

        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);

        carModel.Draw(carShader);

        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDisable(GL_DEPTH_TEST);
        carLineShader.use();
        carLineShader.setFloat("outlining", 0.029f);
        carLineShader.setMat4("projection", car_projection);
        carLineShader.setMat4("view", car_view);
        carLineShader.setMat4("model", car_model);
        carModel.Draw(carLineShader);
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        glEnable(GL_DEPTH_TEST);

        ////////////////////////////////////////////////////
        //                                                //
        //              Crtanje modela sata               //
        //                                                //
        ////////////////////////////////////////////////////
        clockShader.use();
        clockShader.setVec3("pointLight.position", pointLight.position);
        clockShader.setVec3("pointLight.ambient", pointLight.ambient);
        clockShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        clockShader.setVec3("pointLight.specular", pointLight.specular);
        clockShader.setFloat("pointLight.constant", pointLight.constant);
        clockShader.setFloat("pointLight.linear", pointLight.linear);
        clockShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        clockShader.setVec3("viewPosition", programState->camera.Position);
        clockShader.setFloat("material.shininess", 32.0f);


        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT);
        glFrontFace(GL_CCW);
        
        double currentFrame = currFrame / 1000;
        for (int i = 1; i < 102; i++) {
            glm::mat4 clock_projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                    (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
            glm::mat4 clock_view = programState->camera.GetViewMatrix();
            clockShader.setMat4("projection", clock_projection);
            clockShader.setMat4("view", clock_view);
            glm::mat4 clock_model = glm::mat4(1.0f);
            clock_model = glm::translate(clock_model, 
                programState->backpackPosition + glm::vec3(cos(i * currentFrame) * ((i+1) * currentFrame), 10 + sin(currentFrame * 20) * 2 * cos(currentFrame * 20), 35 * sin(currentFrame * i + 5)));
            clock_model = glm::scale(clock_model, glm::vec3(programState->backpackScale));
            clockShader.setMat4("model", clock_model);
            clockModel.Draw(clockShader);
        }
        glDisable(GL_CULL_FACE);

        ////////////////////////////////////////////////////
        //                                                //
        //              Crtanje modela poda               //
        //                                                //
        ////////////////////////////////////////////////////
        floorShader.use();
        floorShader.setVec3("pointLight.position", pointLight.position);
        floorShader.setVec3("pointLight.ambient", pointLight.ambient);
        floorShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        floorShader.setVec3("pointLight.specular", pointLight.specular);
        floorShader.setFloat("pointLight.constant", pointLight.constant);
        floorShader.setFloat("pointLight.linear", pointLight.linear);
        floorShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        floorShader.setVec3("viewPosition", programState->camera.Position);
        floorShader.setFloat("material.shininess", 32.0f);

        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);
        // glFrontFace(GL_CCW);
        

        glm::mat4 floor_projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 floor_view = programState->camera.GetViewMatrix();
        floorShader.setMat4("projection", floor_projection);
        floorShader.setMat4("view", floor_view);
        glm::mat4 floor_model = glm::mat4(1.0f);
        floor_model = glm::translate(floor_model, 
            programState->backpackPosition + glm::vec3(0.0f, -2.0f, 0.0f));
        floor_model = glm::scale(floor_model, glm::vec3(programState->backpackScale * 5));
        floorShader.setMat4("model", floor_model);
        floorModel.Draw(floorShader);

        // glDisable(GL_CULL_FACE);




        ////////////////////////////////////////////////////
        //                                                //
        //              Crtanje modela trave              //
        //                                                //
        ////////////////////////////////////////////////////
        grassShader.use();
        grassShader.setVec3("pointLight.position", pointLight.position);
        grassShader.setVec3("pointLight.ambient", pointLight.ambient);
        grassShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        grassShader.setVec3("pointLight.specular", pointLight.specular);
        grassShader.setFloat("pointLight.constant", pointLight.constant);
        grassShader.setFloat("pointLight.linear", pointLight.linear);
        grassShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        grassShader.setVec3("viewPosition", programState->camera.Position);
        grassShader.setFloat("material.shininess", 32.0f);

        // glEnable(GL_CULL_FACE);
        // glCullFace(GL_FRONT);
        // glFrontFace(GL_CCW);
        

        glm::mat4 grass_projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 grass_view = programState->camera.GetViewMatrix();
        grassShader.setMat4("projection", grass_projection);
        grassShader.setMat4("view", grass_view);
        glm::mat4 grass_model = glm::mat4(1.0f);
        grass_model = glm::translate(grass_model, 
            programState->backpackPosition + glm::vec3(0.0f, -2.0f, 0.0f));
        grass_model = glm::scale(grass_model, glm::vec3(programState->backpackScale * 5));
        grassShader.setMat4("model", grass_model);
        
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        grassModel.Draw(grassShader);
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);



		glBindFramebuffer(GL_FRAMEBUFFER, 0);


        ////////////////////////////////////////////////////
        //                                                //
        //              Crtanje modela vile               //
        //                                                //
        ////////////////////////////////////////////////////
        villaShader.use();
        villaShader.setVec3("pointLight.position", pointLight.position);
        villaShader.setVec3("pointLight.ambient", pointLight.ambient);
        villaShader.setVec3("pointLight.diffuse", pointLight.diffuse);
        villaShader.setVec3("pointLight.specular", pointLight.specular);
        villaShader.setFloat("pointLight.constant", pointLight.constant);
        villaShader.setFloat("pointLight.linear", pointLight.linear);
        villaShader.setFloat("pointLight.quadratic", pointLight.quadratic);
        villaShader.setVec3("viewPosition", programState->camera.Position);
        villaShader.setFloat("material.shininess", 32.0f);
        glm::mat4 villa_projection = glm::perspective(glm::radians(programState->camera.Zoom),
                                                (float) SCR_WIDTH / (float) SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 villa_view = programState->camera.GetViewMatrix();
        villaShader.setMat4("projection", villa_projection);
        villaShader.setMat4("view", villa_view);
        glm::mat4 villa_model = glm::mat4(1.0f);
        villa_model = glm::translate(villa_model, programState->backpackPosition + glm::vec3(0, 0, 0));
        villa_model = glm::scale(villa_model, glm::vec3(programState->backpackScale));
        villaShader.setMat4("model", villa_model);
        villaModel.Draw(villaShader);

        ////////////////////////////////////////////////////
        //                                                //
        //               Post - Processing                //
        //                                                //
        ////////////////////////////////////////////////////

        if (isPostProcessingEnabled) {
            framebufferShader.use();
            glBindVertexArray(rectVAO);
            glDisable(GL_DEPTH_TEST);
            glBindTexture(GL_TEXTURE_2D, framebufferTexture);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }


        ////////////////////////////////////////////////////
        //                                                //
        //                      GUI                       //
        //                                                //
        ////////////////////////////////////////////////////
        if (programState->ImGuiEnabled)
            DrawImGui(programState);

        ////////////////////////////////////////////////////
        //                                                //
        //                DOUBLE BUFFERING                //
        //                                                //
        ////////////////////////////////////////////////////
        glfwSwapBuffers(window);
        glfwPollEvents();
    }



    //////////////////////////////////////////////////
    //                                              //
    //             Oslobadjanje resursa             //
    //                                              //
    //////////////////////////////////////////////////
    programState->SaveToFile("resources/program_state.txt");
    delete programState;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwTerminate();
    return 0;
}

void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        programState->camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) {
        isPostProcessingEnabled = 1;
    } else {
        isPostProcessingEnabled = 0;
    }

    if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) {
        isHDREnabled = 1;
    } else {
        isHDREnabled = 0;
    }

}

void framebuffer_size_callback(GLFWwindow *window, int width, int height) {
    glViewport(0, 0, width, height);
}

void mouse_callback(GLFWwindow *window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos;

    lastX = xpos;
    lastY = ypos;

    if (programState->CameraMouseMovementUpdateEnabled)
        programState->camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow *window, double xoffset, double yoffset) {
    programState->camera.ProcessMouseScroll(yoffset);
}

void DrawImGui(ProgramState *programState) {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();


    {
        static float f = 0.0f;
        ImGui::Begin("Hello window");
        ImGui::Text("Hello text");
        ImGui::SliderFloat("Float slider", &f, 0.0, 1.0);
        ImGui::ColorEdit3("Background color", (float *) &programState->clearColor);
        ImGui::DragFloat3("Backpack position", (float*)&programState->backpackPosition);
        ImGui::DragFloat("Backpack scale", &programState->backpackScale, 0.05, 0.1, 4.0);

        ImGui::DragFloat("pointLight.constant", &programState->pointLight.constant, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.linear", &programState->pointLight.linear, 0.05, 0.0, 1.0);
        ImGui::DragFloat("pointLight.quadratic", &programState->pointLight.quadratic, 0.05, 0.0, 1.0);
        ImGui::End();
    }

    {
        ImGui::Begin("Camera info");
        const Camera& c = programState->camera;
        ImGui::Text("Camera position: (%f, %f, %f)", c.Position.x, c.Position.y, c.Position.z);
        ImGui::Text("(Yaw, Pitch): (%f, %f)", c.Yaw, c.Pitch);
        ImGui::Text("Camera front: (%f, %f, %f)", c.Front.x, c.Front.y, c.Front.z);
        ImGui::Checkbox("Camera mouse update", &programState->CameraMouseMovementUpdateEnabled);
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_F1 && action == GLFW_PRESS) {
        programState->ImGuiEnabled = !programState->ImGuiEnabled;
        if (programState->ImGuiEnabled) {
            programState->CameraMouseMovementUpdateEnabled = false;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    }
}
