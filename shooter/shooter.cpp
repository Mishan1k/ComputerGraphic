// Include standard headers
#include <stdio.h>
#include <stdlib.h>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <GLFW/glfw3.h>

GLFWwindow *window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

using namespace glm;

#include <vector>
#include <iostream>

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/objloader.hpp>

struct Camera {
    static glm::mat4 ViewMatrix;
    static glm::mat4 ProjectionMatrix;
    // Initial position : on +Z
    static glm::vec3 position;
    // Initial horizontal angle : toward -Z
    static float horizontalAngle;
    // Initial vertical angle : none
    static float verticalAngle;
    // Initial Field of View
    static float initialFoV;

    constexpr static const float speed = 5.0f;
    constexpr static const float mouseSpeed = 0.05f;

    static glm::vec3 direction;

    static void computeMatricesFromInputs();
};

glm::mat4 Camera::ViewMatrix = glm::mat4();
glm::mat4 Camera::ProjectionMatrix = glm::mat4();
glm::vec3 Camera::position = glm::vec3(-11, -11, -11);
float Camera::horizontalAngle = glm::asin(0.625f);
float Camera::verticalAngle = glm::asin(0.6f);
float Camera::initialFoV = 45.0f;
glm::vec3 Camera::direction = glm::vec3(-0.6f, -0.6f, -0.6f);
const size_t MAX_FIREBALL_NUM = 10;


void Camera::computeMatricesFromInputs() {

    // glfwGetTime is called only once, the first time this function is called
    static double lastTime = glfwGetTime();

    // Compute time difference between current and last frame
    double currentTime = glfwGetTime();
    auto deltaTime = float(currentTime - lastTime);

    static bool is_first_call = true;
    if (is_first_call) {
        is_first_call = false;
        glfwSetCursorPos(window, 1024.f / 2, 768.f / 2);
    }

    // Get mouse position
    double xPos, yPos;
    glfwGetCursorPos(window, &xPos, &yPos);

    // Reset mouse position for next frame
    glfwSetCursorPos(window, 1024.f / 2, 768.f / 2);

    // Compute new orientation
    horizontalAngle += mouseSpeed * float(1024.f / 2 - xPos);
    verticalAngle += mouseSpeed * float(768.f / 2 - yPos);

    // Direction : Spherical coordinates to Cartesian coordinates conversion
    direction = glm::vec3(
            cos(verticalAngle) * sin(horizontalAngle),
            sin(verticalAngle),
            cos(verticalAngle) * cos(horizontalAngle)
    );

    // side vector
    glm::vec3 side = glm::vec3(
            sin(horizontalAngle - 3.14f / 2.0f),
            0,
            cos(horizontalAngle - 3.14f / 2.0f)
    );

    // Up vector
    glm::vec3 up = glm::cross(side, direction);

    // Move forward
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
        position += direction * deltaTime * speed;
    }
    // Move backward
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
        position -= direction * deltaTime * speed;
    }
    // Move right
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) {
        position += side * deltaTime * speed;
    }
    // move left
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS) {
        position -= side * deltaTime * speed;
    }

    float FoV = initialFoV;// - 5 * glfwGetMouseWheel(); // Now GLFW 3 requires setting up a callback for this. It's a bit too complicated for this beginner's tutorial, so it's disabled instead.
    //  ModelMatrix = glm::translate(glm::mat4(), );
    // Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    ProjectionMatrix = glm::perspective(glm::radians(FoV), 4.0f / 3.0f, 0.1f, 100.0f);
    // Camera matrix
    ViewMatrix = glm::lookAt(
            position,           // Camera is here
            position + direction, // and looks here : at the same position, plus "direction"
            up                  // Head is up (set to 0,-1,0 to look upside-down)
    );

    // For the next frame, the "last time" will be "now"
    lastTime = currentTime;
}

struct Enemy {
    constexpr static const GLfloat colliderRadius = 1.5f;
    constexpr static const GLfloat CREATION_TIME = 0.2f;
    glm::vec3 position;
    GLfloat angle;
    glm::vec3 rotationAxis;

    Enemy() {
        position = {
                ((GLfloat) (std::rand() % 1000)) / 50.0f - 10.f,
                ((GLfloat) (std::rand() % 1000)) / 50.0f - 20.f,
                ((GLfloat) (std::rand() % 10)) -5.f ,
        };
        angle = ((GLfloat) (std::rand() % 360)) / 360 * glm::pi<GLfloat>();
        GLfloat x = ((GLfloat) (std::rand() % 1000)) / 100.0f;
        GLfloat y = (std::rand() % 1000) / 100.0f;
        GLfloat z = (std::rand() % 1000) / 100.0f;
        rotationAxis = {
                x,
                y,
                z
        };
        glm::normalize(rotationAxis);
    }
};

struct Fireball {
    constexpr static const GLfloat colliderRadius = 0.5f;
    constexpr static const GLfloat cooldown = 1.0f;
    constexpr static const GLfloat speed = 0.05f;
    glm::vec3 position_;
    glm::vec3 direction_;

    explicit Fireball() :
            position_(Camera::position + Camera::direction * (colliderRadius * 2.f)),
            direction_(Camera::direction) {}
};

bool deleteCollidedObjects(std::vector<Fireball>::iterator &ball_it,
                           std::vector<Fireball> &balls,
                           std::vector<Enemy> &enemies) {
    for (auto enemy_it = enemies.begin(); enemy_it != enemies.end(); ++enemy_it) {
        auto dist_coords = ball_it->position_ - enemy_it->position;
        GLfloat dist = 0.f;
        for (int i = 0; i < 3; ++i) {
            dist += dist_coords[i] * dist_coords[i];
        }
        if (glm::sqrt(dist) <= Enemy::colliderRadius + Fireball::colliderRadius) {
            ball_it = balls.erase(ball_it);
            enemy_it = enemies.erase(enemy_it);
            return true;
        }
    }
    return false;
}

int main(void) {
    // Initialise GLFW
    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Open a window and create its OpenGL context
    window = glfwCreateWindow(1024, 768, "Shooter", NULL, NULL);
    if (window == NULL) {
        fprintf(stderr,
                "Failed to open GLFW window. If you have an Intel GPU, they are not 3.3 compatible. Try the 2.1 version of the tutorials.\n");
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    glewExperimental = true; // Needed for core profile
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    GLuint VertexArrayID;
    glGenVertexArrays(1, &VertexArrayID);
    glBindVertexArray(VertexArrayID);

    // Create and compile our GLSL program from the shaders
    GLuint programID = LoadShaders("shaders/TransformVertexShader.vertexshader", "shaders/TextureFragmentShader.fragmentshader");

    GLuint ProjectionMatrixID = glGetUniformLocation(programID, "P");
    GLuint ViewMatrixID = glGetUniformLocation(programID, "V");
    GLuint ModelMatrixID = glGetUniformLocation(programID, "M");


    // Load textures
    GLuint fireballTexture = loadDDS("textures/lava.DDS");
    GLuint enemyTexture = loadDDS("textures/stone_lava.DDS");

    // Get a handle for our "myTextureSampler" uniform
    GLuint EnemyTextureID = glGetUniformLocation(programID, "myTextureSampler");
    GLuint BallTextureID = glGetUniformLocation(programID, "myTextureSampler");

    // Read sphere .obj file
    std::vector<glm::vec3> ball_vertices;
    std::vector<glm::vec2> ball_uvs;
    std::vector<glm::vec3> ball_normals;
    assert(loadOBJ("models/fireball_model.obj", ball_vertices, ball_uvs, ball_normals));

    // Read cube .obj file
    std::vector<glm::vec3> enemy_vertices;
    std::vector<glm::vec2> enemy_uvs;
    std::vector<glm::vec3> enemy_normals;
    assert(loadOBJ("models/cube.obj", enemy_vertices, enemy_uvs, enemy_normals));

    // Projection matrix : 45� Field of View, 4:3 ratio, display range : 0.1 unit <-> 100 units
    glm::mat4 Projection = glm::perspective(glm::radians(45.0f), 4.0f / 3.0f, 0.1f, 100.0f);
    // Camera matrix
    glm::mat4 View = glm::lookAt(
            glm::vec3(10, 10, 10),
            glm::vec3(0, 0, 0),
            glm::vec3(0, 1, 0)
    );
    // Model matrix : an identity matrix (model will be at the origin)
    glm::mat4 Model = glm::mat4(1.0f);

    GLuint vertexBuffers[2];
    glGenBuffers(2, vertexBuffers);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, enemy_vertices.size() * sizeof(glm::vec3), &enemy_vertices[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[1]);
    glBufferData(GL_ARRAY_BUFFER, ball_vertices.size() * sizeof(glm::vec3), &ball_vertices[0], GL_STATIC_DRAW);

    GLuint uvBuffers[2];
    glGenBuffers(2, uvBuffers);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, enemy_uvs.size() * sizeof(glm::vec2), &enemy_uvs[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, uvBuffers[1]);
    glBufferData(GL_ARRAY_BUFFER, ball_uvs.size() * sizeof(glm::vec2), &ball_uvs[0], GL_STATIC_DRAW);

    GLuint normalBuffers[2];
    glGenBuffers(2, normalBuffers);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[0]);
    glBufferData(GL_ARRAY_BUFFER, enemy_normals.size() * sizeof(glm::vec3), &enemy_normals[0], GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[1]);
    glBufferData(GL_ARRAY_BUFFER, ball_normals.size() * sizeof(glm::vec3), &ball_normals[0], GL_STATIC_DRAW);

    std::vector<Enemy> enemies;
    std::vector<Fireball> balls;

    // Use our shader
    glUseProgram(programID);


    do {
        // create enemy by time
        static GLfloat lastTime = glfwGetTime();
        GLfloat currentTime = glfwGetTime();
        if (currentTime - lastTime >= Enemy::CREATION_TIME) {
            lastTime = currentTime;
            enemies.emplace_back();
        }

        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            enemies.emplace_back();
        }

        // create ball by key space
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) {
            static GLfloat lastTimeBalls = glfwGetTime() - Fireball::cooldown;
            GLfloat currentTimeBalls = glfwGetTime();
            if (currentTimeBalls - lastTimeBalls >= Fireball::cooldown) {
                lastTimeBalls = currentTimeBalls;
                balls.emplace_back();
            }
        }

        // Clear the screen
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // delete extra balls
        while (balls.size() > MAX_FIREBALL_NUM) {
            balls.erase(balls.begin());
        }



        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, enemyTexture);
        // Set our "myTextureSampler" sampler to use Texture Unit 0
        glUniform1i(EnemyTextureID, 0);

        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[0]);
        glVertexAttribPointer(
                0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void *) 0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffers[0]);
        glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                2,                                // size : U+V => 2
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void *) 0                          // array buffer offset
        );

        // 3rd attribute buffer : Normals
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[0]);
        glVertexAttribPointer(
                2,                                // attribute. No particular reason for 2, but must match the layout in the shader.
                3,                                // size : 3
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void *) 0                          // array buffer offset
        );

        Camera::computeMatricesFromInputs();
        Projection = Camera::ProjectionMatrix;
        View = Camera::ViewMatrix;
        for (const auto &enemy: enemies) {
            Model = glm::translate(glm::mat4(), enemy.position);
            Model = glm::rotate(Model, enemy.angle, enemy.rotationAxis);

            glUniformMatrix4fv(ProjectionMatrixID, 1, GL_FALSE, &Projection[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model[0][0]);
            glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View[0][0]);

            glDrawArrays(GL_TRIANGLES, 0, enemy_vertices.size());
        }
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        //// Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, fireballTexture);
        //// Set our to use Texture Unit 0
        glUniform1i(BallTextureID, 0);



        glEnableVertexAttribArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffers[1]);
        glVertexAttribPointer(
                0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void *) 0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(1);
        glBindBuffer(GL_ARRAY_BUFFER, uvBuffers[1]);
        glVertexAttribPointer(
                1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                2,                                // size : U+V => 2
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void *) 0                          // array buffer offset
        );

        // 3rd attribute buffer : Normals
        glEnableVertexAttribArray(2);
        glBindBuffer(GL_ARRAY_BUFFER, normalBuffers[1]);
        glVertexAttribPointer(
                2,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                3,                                // size : 3
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void *) 0                          // array buffer offset
        );

        for (auto ball_it = balls.begin(); ball_it != balls.end();) {
            ball_it->position_ += ball_it->direction_ * Fireball::speed;
            if (!deleteCollidedObjects(ball_it, balls, enemies)) {
                Model = glm::translate(glm::mat4(), ball_it->position_);

                glUniformMatrix4fv(ProjectionMatrixID, 1, GL_FALSE, &Projection[0][0]);
                glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model[0][0]);
                glUniformMatrix4fv(ViewMatrixID, 1, GL_FALSE, &View[0][0]);

                glDrawArrays(GL_TRIANGLES, 0, ball_vertices.size());
                ++ball_it;
            }
        }
        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
        glDisableVertexAttribArray(2);

        glfwSwapBuffers(window);
        glfwPollEvents();
    } // Check if the ESC key was pressed or the window was closed
    while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
           glfwWindowShouldClose(window) == 0);

    // Cleanup VBO and shader
    glDeleteBuffers(2, vertexBuffers);
    glDeleteBuffers(2, uvBuffers);
    glDeleteProgram(programID);
    glDeleteVertexArrays(1, &VertexArrayID);
    glDeleteTextures(1, &fireballTexture);
    glDeleteTextures(1, &enemyTexture);

    // Close OpenGL window and terminate GLFW
    glfwTerminate();

    return 0;
}

