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

float deltaTime = 0.0f;
float lastFrame = 0.0f;

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

    // stbi_set_flip_vertically_on_load(true);

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



    //////////////////////////////////////////////////
    //                                              //
    //                   Shaderi                    //
    //                                              //
    //////////////////////////////////////////////////
    Shader carLineShader("resources/shaders/carline.vs", "resources/shaders/carline.fs");
    Shader carShader("resources/shaders/default.vs", "resources/shaders/default.fs");
    Shader villaShader("resources/shaders/villa.vs", "resources/shaders/villa.fs");
    Shader clockShader("resources/shaders/clock.vs", "resources/shaders/clock.fs");


    Model villaModel("resources/objects/futuristic_app/Futuristic\ Apartment.obj");
    Model carModel("resources/objects/car/car.obj");
    Model clockModel("resources/objects/clockwork/clock.obj");

    villaModel.SetShaderTextureNamePrefix("material.");
    carModel.SetShaderTextureNamePrefix("material.");
    clockModel.SetShaderTextureNamePrefix("material.");

    PointLight& pointLight = programState->pointLight;
    pointLight.position = glm::vec3(4.0f, 59.0, 0.0);
    pointLight.ambient = glm::vec3(0.1, 0.1, 0.1);
    pointLight.diffuse = glm::vec3(0.6, 0.6, 0.6);
    pointLight.specular = glm::vec3(27.0, 77.0, 255.0);
    pointLight.constant = 1.0f;
    pointLight.linear = 0.09f;
    pointLight.quadratic = 0.031f;


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
        glClearColor(programState->clearColor.r, programState->clearColor.g, programState->clearColor.b, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        // pozicija svetla
        pointLight.position = glm::vec3(20.0 * cos(currFrame), 50 + 50.0f * abs(cos(currFrame)), 20.0 * sin(currFrame));

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
