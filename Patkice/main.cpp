#define _CRT_SECURE_NO_WARNINGS
#define CRES 50 // circle resolution - rezolucija kruga - jezera

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <iostream>
#include <fstream>
#include <sstream>

#include <GL/glew.h> 
#include <GLFW/glfw3.h>

//GLM biblioteke
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <ft2build.h>
#include <map>
#include FT_FREETYPE_H
#include <string>

unsigned int compileShader(GLenum type, const char* source);
unsigned int createShader(const char* vsSource, const char* fsSource);
void RenderText(GLuint shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, GLfloat red, GLfloat green, GLfloat blue);

struct Character {
    GLuint TextureID;
    std::pair<int, int> Size;
    std::pair<int, int> Bearing;
    unsigned int Advance;
};

std::map<GLchar, Character> Characters;
GLuint textVAO, textVBO;

int main(void)
{

    if (!glfwInit())
    {
        std::cout << "GLFW Biblioteka se nije ucitala! :(\n";
        return 1;
    }


    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window;
    unsigned int wWidth = 1920;
    unsigned int wHeight = 1080;
    const char wTitle[] = "Patkice";
    window = glfwCreateWindow(wWidth, wHeight, wTitle, glfwGetPrimaryMonitor(), NULL);

    

    if (window == NULL)
    {
        std::cout << "Prozor nije napravljen! :(\n";
        glfwTerminate();
        return 2;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    glViewport(0, 0, wWidth, wHeight);

    glewExperimental = GL_TRUE;
    if (glewInit() != GLEW_OK)
    {
        std::cout << "GLEW nije mogao da se ucita! :'(\n";
        return 3;
    }
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    unsigned int VAO[5];
    glGenVertexArrays(5, VAO);

    unsigned int VBO[5];
    glGenBuffers(5, VBO);

    unsigned int EBO[1];
    glGenBuffers(1, EBO);


    //shaderi
    unsigned int unifiedShader = createShader("basic.vert", "basic.frag");
    unsigned int lakeShader = createShader("lake.vert", "lake.frag");
    unsigned int duckShader = createShader("duck.vert", "duck.frag");
    unsigned int textShader = createShader("text.vert", "text.frag");
    unsigned int grassShader = createShader("grass.vert", "grass.frag");


    //podesavanje fonta
    std::ifstream file("assets/fonts/Roboto-Light.ttf");
    if (!file) {
        std::cerr << "Cannot find the font file in the specified path." << std::endl;
        return -1;
    }
    file.close();

    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        std::cerr << "Could not init FreeType Library" << std::endl;
        return -1;
    }

    FT_Face face;
    if (FT_New_Face(ft, "assets/fonts/Roboto-Bold.ttf", 0, &face)) {
        std::cerr << "Failed to load font" << std::endl;
        return -1;
    }

    FT_Set_Pixel_Sizes(face, 0, 48);

    //ucitavanje karaktera
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    for (GLubyte c = 0; c < 128; c++) {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
            std::cerr << "Failed to load Glyph" << std::endl;
            continue;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer
        );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        Character character = {
             texture,
             std::make_pair(face->glyph->bitmap.width, face->glyph->bitmap.rows),
             std::make_pair(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<GLuint>(face->glyph->advance.x)
        };
        Characters[c] = character;
    }
    glBindTexture(GL_TEXTURE_2D, 0);
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    float trava[] =
    {
        -30.0f, 0.0f, -30.0f,   0.0f, 0.0f,
         30.0f, 0.0f,  30.0f,   15.0f, 15.0f,
         30.0f, 0.0f, -30.0f,   15.0f, 0.0f,

        -30.0f, 0.0f, -30.0f,   0.0f, 0.0f,
        -30.0f, 0.0f,  30.0f,   0.0f, 15.0f,
         30.0f, 0.0f,  30.0f,   15.0f, 15.0f
    };
    unsigned int travaStride = (3 + 2) * sizeof(float);

    glBindVertexArray(VAO[0]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(trava), trava, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, travaStride, (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, travaStride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint grassTexture;
    glGenTextures(1, &grassTexture);
    glBindTexture(GL_TEXTURE_2D, grassTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    unsigned char* data = stbi_load("res/grass1.jpeg", &width, &height, &nrChannels, 0);
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else {
        std::cout << "Failed to load grass texture" << std::endl;
    }
    stbi_image_free(data);


    // jezero
    float lake[(CRES + 2) * 5];
    float rl = 3.5f;

    lake[0] = 0.0f;
    lake[1] = 0.1f;   
    lake[2] = 0.0f;
    lake[3] = 0.5f;
    lake[4] = 0.5f;

    for (int j = 0; j <= CRES; j++)
    {
        float angle = (float)(CRES - j) / CRES * 2.0f * 3.141592f;

        float x = rl * cos(angle);
        float y = 0.1f;
        float z = rl * sin(angle);

        float u = (x / (2.0f * rl)) + 0.5f;
        float v = (z / (2.0f * rl)) + 0.5f;

        lake[5 + j * 5 + 0] = x;
        lake[5 + j * 5 + 1] = y;
        lake[5 + j * 5 + 2] = z;
        lake[5 + j * 5 + 3] = u;
        lake[5 + j * 5 + 4] = v;
    }

    glBindVertexArray(VAO[1]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(lake), lake, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    GLuint lakeTexture;
    glGenTextures(1, &lakeTexture);
    glBindTexture(GL_TEXTURE_2D, lakeTexture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int widthLake, heightLake, nrChannelsLake;
    unsigned char* dataLake = stbi_load("res/water.jpeg", &widthLake, &heightLake, &nrChannelsLake, 0);
    if (dataLake)
    {
        GLenum format = (nrChannelsLake == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, widthLake, heightLake, 0, format, GL_UNSIGNED_BYTE, dataLake);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load lake texture" << std::endl;
    }
    stbi_image_free(dataLake);


    //patka
    float kocka[] = {
        -0.3f, -0.5f,  0.5f,   // A
         0.3f, -0.5f,  0.5f,   // B
         0.3f,  0.5f,  0.5f,   // F
        -0.3f,  0.5f,  0.5f,   // E

        -0.3f, -0.5f, -0.5f,   // D
         0.3f, -0.5f, -0.5f,   // C
         0.3f,  0.5f, -0.5f,   // G
        -0.3f,  0.5f, -0.5f    // H
    };

    unsigned int indices[] = {
        0, 1, 2,    // A, B, F
        0, 2, 3,    // A F E

        1, 5, 6,    // B, C, G
        1, 6, 2,    // B, G, F

        5, 4, 7,    // C, D, H
        5, 7, 6,    // C H G

        4, 0, 3,    // D, A, E
        4, 3, 7,    // D, E, H

        4, 5, 1,    // D, C, B
        4, 1, 0,    // D, B, A

        3, 2, 6,    // E, F, G
        3, 6, 7     // E, G, H
    };

    glBindVertexArray(VAO[2]);
    glBindBuffer(GL_ARRAY_BUFFER, VBO[2]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kocka), kocka, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO[0]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);

    // kljun
    float kljun[] = {
        0.70f, -0.224f, 0.35f,   // D
        1.05f, -0.224f, 0.35f,   // C
        1.05f, -0.224f, -0.35f,  // B

        0.70f, -0.224f, 0.35f,   // D
        1.05f, -0.224f, -0.35f,  // B
        0.70f, -0.224f, -0.35f,  // E

        0.875f, 0.224f, 0.0f,    // A
        1.05f, -0.224f, -0.35f,  // B
        0.70f, -0.224f, -0.35f,  // E

        0.875f, 0.224f, 0.0f,    // A
        1.05f, -0.224f, 0.35f,   // C
        1.05f, -0.224f, -0.35f,  // B

        0.875f, 0.224f, 0.0f,    // A
        0.70f, -0.224f, 0.35f,   // D
        1.05f, -0.224f, 0.35f,   // C

        0.875f, 0.224f, 0.0f,    // A
        0.70f, -0.224f, -0.35f,  // E
        0.70f, -0.224f, 0.35f,   // D
    };


    unsigned int p3 = (3) * sizeof(float);

    glBindVertexArray(VAO[3]);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[3]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(kljun), kljun, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, p3, (void*)0);
    glEnableVertexAttribArray(0);

    float oci[] = {
        0.50f,  0.224f, 0.1f,   // D
        0.70f, 0.224f, 0.1f,    // C
        0.70f, -0.224f, -0.1f,  // B

        0.50f,  0.224f, 0.1f,   // D
        0.70f, -0.224f, -0.1f,   // B
        0.50f, -0.224f, -0.1f,  // E
    };



    unsigned int o = (3) * sizeof(float);

    glBindVertexArray(VAO[4]);

    glBindBuffer(GL_ARRAY_BUFFER, VBO[4]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(oci), oci, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, o, (void*)0);
    glEnableVertexAttribArray(0);

    //ugao pataka na jezeru
    float radius = 3.0f;
    float angleMom = 0.0f;
    float angleChild1 = glm::radians(20.0f);
    float angleChild2 = glm::radians(40.0f);
    float angleChild3 = glm::radians(60.0f);
    
    float swimSpeed = 0.5f;
    float lastTime = 0.0f;


    // pomeranje kamere
    float cameraAngle = 0.0f;
    float cameraRadius = 6.0f;
    glm::vec3 lakeCenter(0.0f, 1.0f, 0.0f);
    float cameraSpeed = glm::radians(30.0f);
    

    // MODEL VIEW PROJECTION
    glm::mat4 model = glm::mat4(1.0f);

    /*glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 2.0f, 5.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );*/

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f),
        (float)wWidth / (float)wHeight,
        0.1f,
        100.0f
    );


    glClearColor(0.53f, 0.81f, 0.92f, 1.0f);    // boja neba
    while (!glfwWindowShouldClose(window))
    {
        float maxSpeed = 5.5f;
        float minSpeed = 0.1f;
        float speedIncrement = 1.0f;

        float currentTime = glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;


        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        {
            glfwSetWindowShouldClose(window, GL_TRUE);
        }

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            swimSpeed += speedIncrement * deltaTime;;
            if (swimSpeed > maxSpeed)
                swimSpeed = maxSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            swimSpeed -= speedIncrement * deltaTime;
            if (swimSpeed < minSpeed)
                swimSpeed = minSpeed;
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cameraAngle -= cameraSpeed * deltaTime;
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cameraAngle += cameraSpeed * deltaTime;
        }

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS) {
            cameraRadius -= 1.5f * deltaTime;
            if (cameraRadius < 2.0f)
                cameraRadius = 2.0f;
        }

        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS) {
            cameraRadius += 1.5f * deltaTime;
            if (cameraRadius > 10.0f) cameraRadius = 10.0f;
        }

        float camX = cameraRadius * cos(cameraAngle);
        float camZ = cameraRadius * sin(cameraAngle);

        glm::vec3 cameraPos(camX, 2.0f, camZ);
        glm::vec3 cameraTarget = glm::vec3(0.0f, 0.1f, 0.0f);
        glm::vec3 up(0.0f, 1.0f, 0.0f);

        glm::mat4 view = glm::lookAt(cameraPos, cameraTarget, up);

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); 


        //trava
        glUseProgram(grassShader);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, grassTexture);
        glUniform1i(glGetUniformLocation(grassShader, "texture1"), 0);

        glUniformMatrix4fv(glGetUniformLocation(grassShader, "uM"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(grassShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(grassShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[0]);
        glDrawArrays(GL_TRIANGLES, 0, 6);



        //jezero
        glUseProgram(lakeShader);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, lakeTexture);
        glUniform1i(glGetUniformLocation(lakeShader, "texture2"), 0);

        glUniformMatrix4fv(glGetUniformLocation(lakeShader, "uM"), 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix4fv(glGetUniformLocation(lakeShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(lakeShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[1]);
        glDrawArrays(GL_TRIANGLE_FAN, 0, CRES + 2);
        glDepthMask(GL_TRUE);


        //patka
        angleMom -= swimSpeed * deltaTime;
        angleChild1 -= swimSpeed * deltaTime;
        angleChild2 -= swimSpeed * deltaTime;
        angleChild3 -= swimSpeed * deltaTime;

        float y = 0.1f;
        glm::vec3 posMom = glm::vec3(radius * cos(angleMom), y, radius * sin(angleMom));
        glm::vec3 posChild1 = glm::vec3(radius * cos(angleChild1), y, radius * sin(angleChild1));
        glm::vec3 posChild2 = glm::vec3(radius * cos(angleChild2), y, radius * sin(angleChild2));
        glm::vec3 posChild3 = glm::vec3(radius * cos(angleChild3), y, radius * sin(angleChild3));

        // mama
        glm::mat4 modelMom = glm::mat4(1.0f);
        modelMom = glm::translate(modelMom, posMom);
        modelMom = glm::rotate(modelMom, -angleMom, glm::vec3(0.0f, 1.0f, 0.0f));
        modelMom = glm::scale(modelMom, glm::vec3(0.5f, 0.3f, 0.3f));

        // mama glava
        glm::mat4 modelMomHead = modelMom;
        modelMomHead = glm::translate(modelMomHead, glm::vec3(0.0f, 0.7f, -0.5f));
        modelMomHead = glm::scale(modelMomHead, glm::vec3(0.5f, 0.5f, 0.5f));

        // mama kljun
        glm::mat4 modelKljun = modelMomHead;
        modelKljun = glm::translate(modelKljun, glm::vec3(-0.6f, 0.0f, -0.5f));
        modelKljun = glm::scale(modelKljun, glm::vec3(0.7f, 0.7f, 0.7f));

        // mama levo oko
        glm::mat4 modelMamaOko1 = modelMomHead;
        modelMamaOko1 = glm::translate(modelMamaOko1, glm::vec3(0.0f, 0.2f, -0.5f));
        modelMamaOko1 = glm::scale(modelMamaOko1, glm::vec3(0.4f, 0.4f, 0.4f));

        // mama desno oko
        glm::mat4 modelMamaOko2 = modelMomHead;
        modelMamaOko2 = glm::translate(modelMamaOko2, glm::vec3(-0.5f, 0.2f, -0.5f));
        modelMamaOko2 = glm::scale(modelMamaOko2, glm::vec3(0.4f, 0.4f, 0.4f));

        //pace 1
        glm::mat4 modelChild1 = glm::mat4(1.0f);
        modelChild1 = glm::translate(modelChild1, posChild1);
        modelChild1 = glm::rotate(modelChild1, -angleChild1, glm::vec3(0.0f, 1.0f, 0.0f));
        modelChild1 = glm::scale(modelChild1, glm::vec3(0.3f, 0.3f, 0.3f));

        // pace 1 glava
        glm::mat4 modelChild1Head = modelChild1;
        modelChild1Head = glm::translate(modelChild1Head, glm::vec3(0.0f, 0.7f, -0.3f));
        modelChild1Head = glm::scale(modelChild1Head, glm::vec3(0.5f, 0.5f, 0.5f));

        // pace 1 kljun
        glm::mat4 modelKljunDete1 = modelChild1Head;
        modelKljunDete1 = glm::translate(modelKljunDete1, glm::vec3(-0.6f, 0.0f, -0.5f));
        modelKljunDete1 = glm::scale(modelKljunDete1, glm::vec3(0.7f, 0.7f, 0.7f));

        // pace 1 levo oko
        glm::mat4 modelChild1Oko1 = modelChild1Head;
        modelChild1Oko1 = glm::translate(modelChild1Oko1, glm::vec3(0.0f, 0.2f, -0.5f));
        modelChild1Oko1 = glm::scale(modelChild1Oko1, glm::vec3(0.4f, 0.4f, 0.4f));

        // pace 1 desno oko
        glm::mat4 modelChild1Oko2 = modelChild1Head;
        modelChild1Oko2 = glm::translate(modelChild1Oko2, glm::vec3(-0.5f, 0.2f, -0.5f));
        modelChild1Oko2 = glm::scale(modelChild1Oko2, glm::vec3(0.4f, 0.4f, 0.4f));

        //pace 2
        glm::mat4 modelChild2 = glm::mat4(1.0f);
        modelChild2 = glm::translate(modelChild2, posChild2);
        modelChild2 = glm::rotate(modelChild2, -angleChild2, glm::vec3(0.0f, 1.0f, 0.0f));
        modelChild2 = glm::scale(modelChild2, glm::vec3(0.25f, 0.25f, 0.25f));

        // pace 2 glava
        glm::mat4 modelChild2Head = modelChild2;
        modelChild2Head = glm::translate(modelChild2Head, glm::vec3(0.0f, 0.7f, -0.3f));
        modelChild2Head = glm::scale(modelChild2Head, glm::vec3(0.5f, 0.5f, 0.5f));

        // pace 2 kljun
        glm::mat4 modelKljunDete2 = modelChild2Head;
        modelKljunDete2 = glm::translate(modelKljunDete2, glm::vec3(-0.6f, 0.0f, -0.5f));
        modelKljunDete2 = glm::scale(modelKljunDete2, glm::vec3(0.7f, 0.7f, 0.7f));

        // pace 2 levo oko
        glm::mat4 modelChild2Oko1 = modelChild2Head;
        modelChild2Oko1 = glm::translate(modelChild2Oko1, glm::vec3(0.0f, 0.2f, -0.5f));
        modelChild2Oko1 = glm::scale(modelChild2Oko1, glm::vec3(0.4f, 0.4f, 0.4f));

        // pace 2 desno oko
        glm::mat4 modelChild2Oko2 = modelChild2Head;
        modelChild2Oko2 = glm::translate(modelChild2Oko2, glm::vec3(-0.5f, 0.2f, -0.5f));
        modelChild2Oko2 = glm::scale(modelChild2Oko2, glm::vec3(0.4f, 0.4f, 0.4f));

        //pace 3
        glm::mat4 modelChild3 = glm::mat4(1.0f);
        modelChild3 = glm::translate(modelChild3, posChild3);
        modelChild3 = glm::rotate(modelChild3, -angleChild3, glm::vec3(0.0f, 1.0f, 0.0f));
        modelChild3 = glm::scale(modelChild3, glm::vec3(0.2f, 0.2f, 0.2f));

        // pace 3 glava
        glm::mat4 modelChild3Head = modelChild3;
        modelChild3Head = glm::translate(modelChild3Head, glm::vec3(0.0f, 0.7f, -0.3f));
        modelChild3Head = glm::scale(modelChild3Head, glm::vec3(0.5f, 0.5f, 0.5f));

        // pace 3 kljun
        glm::mat4 modelKljunDete3 = modelChild3Head;
        modelKljunDete3 = glm::translate(modelKljunDete3, glm::vec3(-0.6f, 0.0f, -0.5f));
        modelKljunDete3 = glm::scale(modelKljunDete3, glm::vec3(0.7f, 0.7f, 0.7f));

        // pace 3 levo oko
        glm::mat4 modelChild3Oko1 = modelChild3Head;
        modelChild3Oko1 = glm::translate(modelChild3Oko1, glm::vec3(0.0f, 0.2f, -0.5f));
        modelChild3Oko1 = glm::scale(modelChild3Oko1, glm::vec3(0.4f, 0.4f, 0.4f));

        // pace 3 desno oko
        glm::mat4 modelChild3Oko2 = modelChild3Head;
        modelChild3Oko2 = glm::translate(modelChild3Oko2, glm::vec3(-0.5f, 0.2f, -0.5f));
        modelChild3Oko2 = glm::scale(modelChild3Oko2, glm::vec3(0.4f, 0.4f, 0.4f));

        // mama
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 1.0f, 1.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelMom));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));
        
        glBindVertexArray(VAO[2]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // glava mame
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 1.0f, 1.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelMomHead));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[2]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // kljun mama
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 165.0f / 255.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelKljun));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[3]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        // desno oko mama
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 0.0f, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelMamaOko1));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[4]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        // levo oko mama
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 0.0f, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelMamaOko2));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[4]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        //pace 1
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 1.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild1));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[2]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // glava paceta 1
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 1.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild1Head));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[2]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // kljun pace 1
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 165.0f / 255.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelKljunDete1));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[3]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        // desno oko pace 1
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 0.0f, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild1Oko1));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[4]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        // levo oko pace 1
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 0.0f, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild1Oko2));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[4]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        //pace 2
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 1.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild2));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[2]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // glava paceta 2
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 1.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild2Head));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[2]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // kljun pace 2
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 165.0f / 255.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelKljunDete2));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[3]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        // desno oko pace 2
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 0.0f, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild2Oko1));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[4]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        // levo oko pace 2
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 0.0f, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild2Oko2));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[4]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        //pace 3
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 1.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild3));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[2]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // glava paceta 3
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 1.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild3Head));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[2]);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

        // kljun pace 3
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 1.0f, 165.0f / 255.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelKljunDete3));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[3]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        // desno oko pace 3
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 0.0f, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild3Oko1));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[4]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        // levo oko pace 3
        glUseProgram(duckShader);
        glUniform4f(glGetUniformLocation(duckShader, "uniColor"), 0.0f, 0.0f, 0.0f, 1.0f);
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uM"), 1, GL_FALSE, glm::value_ptr(modelChild3Oko2));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uV"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(duckShader, "uP"), 1, GL_FALSE, glm::value_ptr(projection));

        glBindVertexArray(VAO[4]);
        glDrawArrays(GL_TRIANGLES, 0, 18);

        glBindVertexArray(0);

        glDisable(GL_DEPTH_TEST);
        RenderText(textShader, "Marija Mandic, RA75/2021", 10.0f, 100.0f, 1.0f, 1.0f, 1.0f, 1.0f);

        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // pospremanje
    glDeleteBuffers(1, &textVBO);
    glDeleteVertexArrays(1, &textVAO);
    glDeleteBuffers(1, EBO);
    glDeleteBuffers(5, VBO);
    glDeleteVertexArrays(5, VAO);
    glDeleteProgram(unifiedShader);
    glDeleteProgram(lakeShader);
    glDeleteProgram(duckShader);
    glDeleteProgram(textShader);

    glfwTerminate();
    return 0;
}

unsigned int compileShader(GLenum type, const char* source)
{
    std::string content = "";
    std::ifstream file(source);
    std::stringstream ss;
    if (file.is_open())
    {
        ss << file.rdbuf();
        file.close();
        std::cout << "Uspjesno procitao fajl sa putanje \"" << source << "\"!" << std::endl;
    }
    else {
        ss << "";
        std::cout << "Greska pri citanju fajla sa putanje \"" << source << "\"!" << std::endl;
    }
    std::string temp = ss.str();
    const char* sourceCode = temp.c_str();

    int shader = glCreateShader(type);

    int success;
    char infoLog[512];
    glShaderSource(shader, 1, &sourceCode, NULL);
    glCompileShader(shader);

    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        if (type == GL_VERTEX_SHADER)
            printf("VERTEX");
        else if (type == GL_FRAGMENT_SHADER)
            printf("FRAGMENT");
        printf(" sejder ima gresku! Greska: \n");
        printf(infoLog);
    }
    return shader;
}
unsigned int createShader(const char* vsSource, const char* fsSource)
{
    unsigned int program;
    unsigned int vertexShader;
    unsigned int fragmentShader;

    program = glCreateProgram();

    vertexShader = compileShader(GL_VERTEX_SHADER, vsSource);
    fragmentShader = compileShader(GL_FRAGMENT_SHADER, fsSource);

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);

    glLinkProgram(program);
    glValidateProgram(program);

    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_VALIDATE_STATUS, &success);
    if (success == GL_FALSE)
    {
        glGetShaderInfoLog(program, 512, NULL, infoLog);
        std::cout << "Objedinjeni sejder ima gresku! Greska: \n";
        std::cout << infoLog << std::endl;
    }

    glDetachShader(program, vertexShader);
    glDeleteShader(vertexShader);
    glDetachShader(program, fragmentShader);
    glDeleteShader(fragmentShader);

    return program;
}


void RenderText(GLuint shader, std::string text, GLfloat x, GLfloat y, GLfloat scale, GLfloat red, GLfloat green, GLfloat blue) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    unsigned int wWidth = 1920;
    unsigned int wHeight = 1080;

    glUseProgram(shader);
    glUniform1i(glGetUniformLocation(shader, "text"), 0);
    glm::mat4 orthoProj = glm::ortho(
        0.0f, static_cast<float>(wWidth),
        0.0f, static_cast<float>(wHeight)
    );
    glUniformMatrix4fv(glGetUniformLocation(shader, "projection"), 1, GL_FALSE, glm::value_ptr(orthoProj));
    glUniform3f(glGetUniformLocation(shader, "textColor"), red, green, blue);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);


    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) {
        if (Characters.find(*c) == Characters.end()) {
            std::cerr << "Karakter nije ucitan: " << *c << std::endl;
            continue;
        }

        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.first * scale;
        GLfloat ypos = y - (ch.Size.second - ch.Bearing.second) * scale;

        GLfloat w = ch.Size.first * scale;
        GLfloat h = ch.Size.second * scale;

        GLfloat vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }
        };



        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        std::cout << "Crtam karakter: " << *c << " na x=" << xpos << ", y=" << ypos << std::endl;

        x += (ch.Advance >> 6) * scale;
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);

}
