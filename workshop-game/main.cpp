
#include <stdio.h>
#include <chrono>
#include <thread>
#include <vector>

#include "imgui/imgui.h"
#include "imgui_impl_glfw_game.h"
#include "imgui_impl_opengl3_game.h"
#include "glad/gl.h"
#include "GLFW/glfw3.h"
#include "draw_game.h"

#include "box2d/box2d.h"
#include <iostream>
#include <random>

// GLFW main window pointer
GLFWwindow* g_mainWindow = nullptr;

b2World* g_world;

class Character;

std::vector<std::unique_ptr<Character>> vecBoxes;

class Character {
public:
    Character(int p_size, float x, float y)
    {
        size = p_size;
        // construct the body of our character
        b2PolygonShape box_shape;
        box_shape.SetAsBox(size / 500.0f, size / 500.0f);
        b2FixtureDef box_fd;
        box_fd.shape = &box_shape;
        box_fd.density = 20.0f;
        box_fd.friction = 0.1f;
        b2BodyDef box_bd;
        // stores the points to this object on the b2Body
        box_bd.userData.pointer = (uintptr_t)this;
        box_bd.type = b2_dynamicBody;
        box_bd.position.Set(x, y);
        body = g_world->CreateBody(&box_bd);
        body->CreateFixture(&box_fd);

    }
    bool toBeDeleted = false;
    const void toDelete() {
        toBeDeleted = true;
    }
    void Jump(float force) {
        std::cout << "Jump with force " << force << "\n";
        this->body->ApplyForceToCenter({0, force}, true);
    }
    void OnCollision(Character* other)
    {
        std::cout << "Two colliding objects size " << this->size << " and size " << other->size << "\n";
        if (this->size > other->size) {
            this->toDelete();
            std::cout << "Delete this object" << "\n";
        }
    }
    virtual ~Character()
    {
        // remove body when object is deleted
        g_world->DestroyBody(body);
    }

private:
    int size;
    b2Body* body; // the body of out character
};

class MyCollisionListener : public b2ContactListener {
public:
    void BeginContact(b2Contact* contact) override
    {
        // if any of the two colliding bodies are not Characters (i.e. it is the floor object), stop processing
        if (contact->GetFixtureA()->GetBody()->GetUserData().pointer == NULL ||
            contact->GetFixtureB()->GetBody()->GetUserData().pointer == NULL
            ) {
            // stop processing
            return;
        }
        // retrieve our two character objects
        Character* character1 = (Character*)contact->GetFixtureA()->GetBody()->GetUserData().pointer;
        Character* character2 = (Character*)contact->GetFixtureB()->GetBody()->GetUserData().pointer;
        // call our new function
        character1->OnCollision(character2);
        character2->OnCollision(character1);
    }
};

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    // code for keys here https://www.glfw.org/docs/3.3/group__keys.html
    // and modifiers https://www.glfw.org/docs/3.3/group__mods.html
    if (action == GLFW_PRESS) {
        // Handle key press
        std::cout << "Key pressed" << action << "\n";

        if (key == GLFW_KEY_SPACE) {
            std::cout << "Clicked space" << "\n";
            for (auto& box : vecBoxes) {
                Character* thisPtr = box.get();
                thisPtr->Jump(2000);
            }
        }
    }
}

void MouseMotionCallback(GLFWwindow*, double xd, double yd)
{
    // get the position where the mouse was pressed
    b2Vec2 ps((float)xd, (float)yd);
    // now convert this position to Box2D world coordinates
    b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);
}

void MouseButtonCallback(GLFWwindow* window, int32 button, int32 action, int32 mods)
{
    // code for mouse button keys https://www.glfw.org/docs/3.3/group__buttons.html
    // and modifiers https://www.glfw.org/docs/3.3/group__buttons.html
    // action is either GLFW_PRESS or GLFW_RELEASE
    double xd, yd;
    // get the position where the mouse was pressed
    glfwGetCursorPos(g_mainWindow, &xd, &yd);
    b2Vec2 ps((float)xd, (float)yd);
    // now convert this position to Box2D world coordinates
    b2Vec2 pw = g_camera.ConvertScreenToWorld(ps);

    if (action == GLFW_RELEASE) {
        printf("Spawn box \n");
        // Create a random number generator engine
        std::default_random_engine generator(std::random_device{}());

        // Create a uniform integer distribution for the range [100, 300]
        std::uniform_int_distribution<int> distribution(100, 300);

        // Generate a random integer
        int randomValue = distribution(generator);
        vecBoxes.emplace_back(std::make_unique<Character>(randomValue, pw.x, pw.y));
    }
}

int main()
{

    // glfw initialization things
    if (glfwInit() == 0) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return -1;
    }

    g_mainWindow = glfwCreateWindow(g_camera.m_width, g_camera.m_height, "My game", NULL, NULL);

    if (g_mainWindow == NULL) {
        fprintf(stderr, "Failed to open GLFW g_mainWindow.\n");
        glfwTerminate();
        return -1;
    }

    // Set callbacks using GLFW
    glfwSetMouseButtonCallback(g_mainWindow, MouseButtonCallback);
    glfwSetKeyCallback(g_mainWindow, KeyCallback);
    glfwSetCursorPosCallback(g_mainWindow, MouseMotionCallback);

    glfwMakeContextCurrent(g_mainWindow);

    // Load OpenGL functions using glad
    int version = gladLoadGL(glfwGetProcAddress);

    // Setup Box2D world and with some gravity
    b2Vec2 gravity;
    gravity.Set(0.0f, -10.0f);
    g_world = new b2World(gravity);

    // Create debug draw. We will be using the debugDraw visualization to create
    // our games. Debug draw calls all the OpenGL functions for us.
    g_debugDraw.Create();
    g_world->SetDebugDraw(&g_debugDraw);
    CreateUI(g_mainWindow, 20.0f /* font size in pixels */);

    // Collision listener
    MyCollisionListener collision;
    g_world->SetContactListener(&collision);

    // Some starter objects are created here, such as the ground
    b2Body* ground;
    b2EdgeShape ground_shape;
    ground_shape.SetTwoSided(b2Vec2(-40.0f, 0.0f), b2Vec2(40.0f, 0.0f));
    b2BodyDef ground_bd;
    ground = g_world->CreateBody(&ground_bd);
    ground->CreateFixture(&ground_shape, 0.0f);

    b2Body* box;
    b2PolygonShape box_shape;
    box_shape.SetAsBox(1.0f, 1.0f);
    b2FixtureDef box_fd;
    box_fd.shape = &box_shape;
    box_fd.density = 20.0f;
    box_fd.friction = 0.1f;
    b2BodyDef box_bd;
    box_bd.type = b2_dynamicBody;
    box_bd.position.Set(-10.0f, 10.0f);
    box_bd.gravityScale = 1;
    box_bd.angularVelocity = 5;
    box_bd.linearVelocity = { 5, 0 };
    box = g_world->CreateBody(&box_bd);
    box->CreateFixture(&box_fd);

    // dominos 
    b2PolygonShape shape;
    shape.SetAsBox(0.1f, 1.0f);

    b2FixtureDef fd;
    fd.shape = &shape;
    fd.density = 20.0f;
    fd.friction = 0.1f;

    for (int i = 0; i < 40; ++i)
    {
        b2BodyDef bd;
        bd.type = b2_dynamicBody;
        bd.position.Set(10.0f + 1.0f * i, 1.25f);
        b2Body* body = g_world->CreateBody(&bd);
        body->CreateFixture(&fd);
    }


    // This is the color of our background in RGB components
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    // Control the frame rate. One draw per monitor refresh.
    std::chrono::duration<double> frameTime(0.0);
    std::chrono::duration<double> sleepAdjust(0.0);

    // Main application loop
    while (!glfwWindowShouldClose(g_mainWindow)) {
        // Use std::chrono to control frame rate. Objective here is to maintain
        // a steady 60 frames per second (no more, hopefully no less)
        std::chrono::steady_clock::time_point t1 = std::chrono::steady_clock::now();

        glfwGetWindowSize(g_mainWindow, &g_camera.m_width, &g_camera.m_height);

        int bufferWidth, bufferHeight;
        glfwGetFramebufferSize(g_mainWindow, &bufferWidth, &bufferHeight);
        glViewport(0, 0, bufferWidth, bufferHeight);

        // Clear previous frame (avoid creates shadows)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Setup ImGui attributes so we can draw text on the screen. Basically
        // create a window of the size of our viewport.
        ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
        ImGui::SetNextWindowSize(ImVec2(float(g_camera.m_width), float(g_camera.m_height)));
        ImGui::SetNextWindowBgAlpha(0.0f);
        ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoScrollbar);
        ImGui::End();

        // Enable objects to be draw
        uint32 flags = 0;
        flags += b2Draw::e_shapeBit;
        g_debugDraw.SetFlags(flags);

        // When we call Step(), we run the simulation for one frame
        float timeStep = 60 > 0.0f ? 1.0f / 60 : float(0.0f);
        g_world->Step(timeStep, 8, 3);

        // Delete marked for delete objects
        for (auto &uniquePtr : vecBoxes) {
            Character* thisPtr = uniquePtr.get();
            if (thisPtr != nullptr && thisPtr->toBeDeleted) {
                vecBoxes.erase(std::remove_if(vecBoxes.begin(), vecBoxes.end(),
                    [thisPtr](const std::unique_ptr<Character>& ptr) {
                        return ptr.get() == thisPtr;
                    }
                ), vecBoxes.end());
            }
        }

        // Render everything on the screen
        g_world->DebugDraw();
        g_debugDraw.Flush();
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(g_mainWindow);

        // Process events (mouse and keyboard) and call the functions we
        // registered before.
        glfwPollEvents();

        // Throttle to cap at 60 FPS. Which means if it's going to be past
        // 60FPS, sleeps a while instead of doing more frames.
        std::chrono::steady_clock::time_point t2 = std::chrono::steady_clock::now();
        std::chrono::duration<double> target(1.0 / 60.0);
        std::chrono::duration<double> timeUsed = t2 - t1;
        std::chrono::duration<double> sleepTime = target - timeUsed + sleepAdjust;
        if (sleepTime > std::chrono::duration<double>(0)) {
            // Make the framerate not go over by sleeping a little.
            std::this_thread::sleep_for(sleepTime);
        }
        std::chrono::steady_clock::time_point t3 = std::chrono::steady_clock::now();
        frameTime = t3 - t1;

        // Compute the sleep adjustment using a low pass filter
        sleepAdjust = 0.9 * sleepAdjust + 0.1 * (target - frameTime);

    }

    // Terminate the program if it reaches here
    glfwTerminate();
    g_debugDraw.Destroy();
    delete g_world;

    return 0;
}
