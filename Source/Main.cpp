#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "../Header/Util.h"
// Main fajl funkcija sa osnovnim komponentama OpenGL programa

// Projekat je dozvoljeno pisati počevši od ovog kostura
// Toplo se preporučuje razdvajanje koda po fajlovima (i eventualno potfolderima) !!!
// Srećan rad!

#include <cmath>

const int NUM_SEGMENTS = 30;
float lampVertices[(NUM_SEGMENTS + 2) * 4];   // X, Y, dummyU, dummyV


int screenWidth = 800;
int screenHeight = 800;
unsigned watermarkTexture;
GLFWcursor* cursorToken;
GLFWcursor* cursorLever;
unsigned int texAnimal1;
unsigned int texAnimal2;
unsigned int texClawOpen;
unsigned int texClawClosed;
bool gameActive = false;      // da li je ubačen žeton
bool mousePressed = false;    // da ne registruje višestruke klikove
float clawX = 0.0f;            // centar kretanja po X
const float clawSpeed = 0.01f; // brzina pomeranja
const float leftLimit = -0.8;
const float rightLimit = 0.8f;
unsigned int VBOcable;
unsigned int VBOclaw;
bool clawDescending = false;
bool clawAscending = false;
float clawY = 0.48f;   // trenutna Y pozicija vrha kandže
float cableTopY = 0.60f;
float clawBottomLimit = -0.38f;   // dno kutije
float clawDropSpeed = 0.01f;

bool grabbed = false;
int grabbedAnimal = -1;

//prate pomeranje zivotinje gore dole
float animal1Yoffset = 0.0f;
float animal2Yoffset = 0.0f;
//prate pomeranje zivotinje levo desno kad je ugrabljena
float animal1Xoffset = 0.0f;
float animal2Xoffset = 0.0f;
//ispustanje zivotinje
bool animalFalling = false;

bool fallingToOutput = false;   // da li životinja pada kroz rupu u donji (crveni) deo

// geometrija rupe (prema dropSlotVertices)
const float holeLeft = 0.36f;
const float holeRight = 0.90f;
const float holeTopY = -0.48f;

// donji deo (prozirni crveni) – malo iznad -0.85 da ne ulazi u ivicu
const float outputFloorY = -0.82f;


bool clawLocked = false;       // kandža ne može da se otvori dok je osvojena životinja
bool lampBlinking = false;     // da li lampica menja boje
double lampTimer = 0.0;        // vreme početka blinkanja


void preprocessTexture(unsigned& texture, const char* filepath) {
    texture = loadImageToTexture(filepath); // Učitavanje teksture
    glBindTexture(GL_TEXTURE_2D, texture); // Vezujemo se za teksturu kako bismo je podesili

    // Generisanje mipmapa - predefinisani različiti formati za lakše skaliranje po potrebi (npr. da postoji 32 x 32 verzija slike, ali i 16 x 16, 256 x 256...)
    glGenerateMipmap(GL_TEXTURE_2D);

    // Podešavanje strategija za wrap-ovanje - šta da radi kada se dimenzije teksture i poligona ne poklapaju
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); // S - tekseli po x-osi
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); // T - tekseli po y-osi

    // Podešavanje algoritma za smanjivanje i povećavanje rezolucije: nearest - bira najbliži piksel, linear - usrednjava okolne piksele
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void drawWatermark(unsigned int watermarkShader, unsigned int VAOwatermark) {
    glUseProgram(watermarkShader); // Podešavanje da se crta koristeći šejder za četvorougao
   
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, watermarkTexture);

    glBindVertexArray(VAOwatermark); // Podešavanje da se crta koristeći vertekse četvorougla
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // Crtaju se trouglovi tako da formiraju četvorougao
}

void formVAOs(float* verticesWatermark, size_t watermarkSize, unsigned int& VAOwatermark) {
    unsigned int VBOrect;
    glGenVertexArrays(1, &VAOwatermark);
    glGenBuffers(1, &VBOrect);

    glBindVertexArray(VAOwatermark);
    glBindBuffer(GL_ARRAY_BUFFER, VBOrect);
    glBufferData(GL_ARRAY_BUFFER, watermarkSize, verticesWatermark, GL_STATIC_DRAW);

    // Atribut 0 (pozicija):
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

}

void resetGame() {
    gameActive = false;
    mousePressed = false;

    // kandža
    clawX = 0.0f;
    clawY = 0.48f;
    clawDescending = false;
    clawAscending = false;
    clawLocked = false;

    // životinje vrati na originalne pozicije
    animal1Xoffset = 0.0f;
    animal1Yoffset = 0.0f;
    animal2Xoffset = 0.0f;
    animal2Yoffset = 0.0f;

    grabbed = false;
    grabbedAnimal = -1;
    animalFalling = false;
    fallingToOutput = false;

    lampBlinking = false;
}


int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Formiranje prozora za prikaz sa datim dimenzijama i naslovom
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();
    const GLFWvidmode* mode = glfwGetVideoMode(monitor);
    screenWidth = mode->width;
    screenHeight = mode->height;
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "Claw", monitor, NULL);

    //GLFWwindow* window = glfwCreateWindow(960, 530, "Kostur", NULL, NULL);

    if (window == NULL) return endProgram("Prozor nije uspeo da se kreira.");
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) return endProgram("GLEW nije uspeo da se inicijalizuje.");

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glClearColor(0.2f, 0.8f, 0.6f, 1.0f);

    preprocessTexture(watermarkTexture, "Resources/Watermark.png");
    preprocessTexture(texAnimal1, "Resources/monkey.png");
    preprocessTexture(texAnimal2, "Resources/bear.png");
    preprocessTexture(texClawOpen, "Resources/handOpened.png");
    preprocessTexture(texClawClosed, "Resources/handClosed.png");

    unsigned int watermarkShader = createShader("watermark.vert", "watermark.frag");
    glUseProgram(watermarkShader);
    glUniform1i(glGetUniformLocation(watermarkShader, "uTex"), 0);

    unsigned int boxShader = createShader("box.vert", "box.frag");

    unsigned int animalShader = createShader("animal.vert", "animal.frag");
    glUseProgram(animalShader);
    glUniform1i(glGetUniformLocation(animalShader, "uTex"), 0);

    float verticesWatermark[] = {
        //  X,     Y,     U,    V
     -0.9f,  0.9f,    0.0f, 1.0f,// gornje levo teme
     -0.9f, 0.75f,    0.0f, 0.0f,// donje levo teme
     -0.61f, 0.75f,    1.0f, 0.0f, // donje desno teme
     -0.61f,  0.9f,    1.0f, 1.0f// gornje desno teme
    };

    float boxVertices[] = {
        //  X,      Y,      dummyU, dummyV
        -0.9f,  0.6f,   0.0f, 0.0f,   // top left
        -0.9f, -0.5f,   0.0f, 0.0f,   // bottom left
         0.9f, -0.5f,   0.0f, 0.0f,   // bottom right
         0.9f,  0.6f,   0.0f, 0.0f    // top right
    };

    float platformVertices[] = {
        // X,      Y,       dummyU, dummyV
        -0.9f, -0.5f,   0.0f, 0.0f,   // top left
        -0.9f, -0.85f,  0.0f, 0.0f,   // bottom left
         0.9f, -0.85f,  0.0f, 0.0f,   // bottom right
         0.9f, -0.5f,   0.0f, 0.0f    // top right
    };

    float slotVertices[] = {
        // X,        Y,         dummyU, dummyV
        -0.5f, -0.53f,   0.0f, 0.0f,   // top left
        -0.5f, -0.62f,   0.0f, 0.0f,   // bottom left
        -0.3f, -0.62f,   0.0f, 0.0f,   // bottom right
        -0.3f, -0.53f,   0.0f, 0.0f    // top right
    };

    float outputZoneVertices[] = {
        // X,      Y,       dummyU, dummyV
         0.9f, -0.6f,    0.0f, 0.0f,   // top left
         0.9f, -0.85f,   0.0f, 0.0f,   // bottom left
         0.90f, -0.85f,   0.0f, 0.0f,   // bottom right
         0.90f, -0.6f,    0.0f, 0.0f    // top right
    };

    float dropSlotVertices[] = {
        // X,      Y,       dummyU, dummyV
         0.36f, -0.48f,   0.0f, 0.0f,  
         0.36f, -0.51f,   0.0f, 0.0f,  
         0.90f, -0.51f,   0.0f, 0.0f,   
         0.90f, -0.48f,   0.0f, 0.0f  
    };

    float animal1Vertices[] = {
        //   X,      Y,      U, V
        -0.55f, -0.2f,   0.0f, 1.0f,   // gornje levo
        -0.55f, -0.53f,   0.0f, 0.0f,   // donje levo
        -0.35f, -0.53f,   1.0f, 0.0f,   // donje desno
        -0.35f, -0.2f,   1.0f, 1.0f    // gornje desno
    };


    float animal2Vertices[] = {
        0.10f, -0.2f,   0.0f, 1.0f,    // gornje levo
        0.10f, -0.53f,   0.0f, 0.0f,    // donje levo
        0.30f, -0.53f,   1.0f, 0.0f,    // donje desno
        0.30f, -0.2f,   1.0f, 1.0f     // gornje desno
    };

    float cableVertices[] = {
    -0.01f,  0.60f,   0.0f, 0.0f,
    -0.01f,  0.48f,   0.0f, 0.0f,
     0.01f,  0.48f,   0.0f, 0.0f,
     0.01f,  0.60f,   0.0f, 0.0f
    };

    float clawVertices[] = {
        -0.06f, 0.48f,    0.0f, 1.0f,
        -0.06f, 0.36f,    0.0f, 0.0f,
         0.06f, 0.36f,    1.0f, 0.0f,
         0.06f, 0.48f,    1.0f, 1.0f
    };

    float centerX = 0.0f;
    float centerY = 0.75f;
    float radius = 0.07f;

    lampVertices[0] = centerX;
    lampVertices[1] = centerY;
    lampVertices[2] = 0.0f;
    lampVertices[3] = 0.0f;

    for (int i = 0; i <= NUM_SEGMENTS; i++)
    {
        float angle = i * 2.0f * 3.1415926f / NUM_SEGMENTS;
        float x = centerX + radius * cos(angle);
        float y = centerY + radius * sin(angle);

        lampVertices[(i + 1) * 4 + 0] = x;
        lampVertices[(i + 1) * 4 + 1] = y;
        lampVertices[(i + 1) * 4 + 2] = 0.0f;
        lampVertices[(i + 1) * 4 + 3] = 0.0f;
    }


    unsigned int VAOwatermark;
    formVAOs(verticesWatermark, sizeof(verticesWatermark), VAOwatermark);


    unsigned int VAObox;
    formVAOs(boxVertices, sizeof(boxVertices), VAObox);

    unsigned int VAOplatform;
    formVAOs(platformVertices, sizeof(platformVertices), VAOplatform);

    unsigned int VAOslot;
    formVAOs(slotVertices, sizeof(slotVertices), VAOslot);

    unsigned int VAOoutput;
    formVAOs(outputZoneVertices, sizeof(outputZoneVertices), VAOoutput);

    unsigned int VAOlamp;
    formVAOs(lampVertices, sizeof(lampVertices), VAOlamp);

    unsigned int VAOdropslot;
    formVAOs(dropSlotVertices, sizeof(dropSlotVertices), VAOdropslot);

    //zivotinja 1
    unsigned int VAOanimal1, VBOanimal1;
    glGenVertexArrays(1, &VAOanimal1);
    glGenBuffers(1, &VBOanimal1);

    glBindVertexArray(VAOanimal1);
    glBindBuffer(GL_ARRAY_BUFFER, VBOanimal1);
    glBufferData(GL_ARRAY_BUFFER, sizeof(animal1Vertices), animal1Vertices, GL_DYNAMIC_DRAW);
    
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // zivotinja 2
    unsigned int VAOanimal2, VBOanimal2;
    glGenVertexArrays(1, &VAOanimal2);
    glGenBuffers(1, &VBOanimal2);

    glBindVertexArray(VAOanimal2);
    glBindBuffer(GL_ARRAY_BUFFER, VBOanimal2);
    glBufferData(GL_ARRAY_BUFFER, sizeof(animal2Vertices), animal2Vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    //  Bounding box pozicije životinja 
    float animal1Left = -0.55f;
    float animal1Right = -0.35f;
    float animal1Top = -0.20f;
    float animal1Bottom = -0.53f;

    float animal2Left = 0.10f;
    float animal2Right = 0.30f;
    float animal2Top = -0.20f;
    float animal2Bottom = -0.53f;


    unsigned int VAOcable;
    glGenVertexArrays(1, &VAOcable);
    glGenBuffers(1, &VBOcable);

    glBindVertexArray(VAOcable);
    glBindBuffer(GL_ARRAY_BUFFER, VBOcable);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cableVertices), cableVertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    unsigned int VAOclaw;
    glGenVertexArrays(1, &VAOclaw);
    glGenBuffers(1, &VBOclaw);

    glBindVertexArray(VAOclaw);
    glBindBuffer(GL_ARRAY_BUFFER, VBOclaw);
    glBufferData(GL_ARRAY_BUFFER, sizeof(clawVertices), clawVertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);

    cursorToken = loadImageToCursor("Resources/handCursor.png");
    cursorLever = loadImageToCursor("Resources/lever.png");
    glfwSetCursor(window, cursorToken);
   

    while (!glfwWindowShouldClose(window))
    {
        double initFrameTime = glfwGetTime();
        glClear(GL_COLOR_BUFFER_BIT);

        if (gameActive) glfwSetCursor(window, cursorLever);
        else glfwSetCursor(window, cursorToken);

        double mx, my;
        glfwGetCursorPos(window, &mx, &my);

        float ndcX = (mx / screenWidth) * 2.0f - 1.0f;
        float ndcY = 1.0f - (my / screenHeight) * 2.0f;

        bool leftClick = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

        bool clickEvent = false;

        if (leftClick && !mousePressed) {
            clickEvent = true;     
            mousePressed = true;
        }

        if (!leftClick) {
            mousePressed = false;
        }

        //ako je kraj igre izvucena zivotinja
        if (clickEvent)
        {
            // 1) Slot za žeton
            bool insideSlot =
                ndcX >= -0.5f && ndcX <= -0.3f &&
                ndcY <= -0.53f && ndcY >= -0.62f;

            if (insideSlot && !gameActive) {
                gameActive = true;
            }

            // 2) Klik na životinju u output zoni
            float a1Left = animal1Left + animal1Xoffset;
            float a1Right = animal1Right + animal1Xoffset;
            float a1Top = animal1Top + animal1Yoffset;
            float a1Bottom = animal1Bottom + animal1Yoffset;

            float a2Left = animal2Left + animal2Xoffset;
            float a2Right = animal2Right + animal2Xoffset;
            float a2Top = animal2Top + animal2Yoffset;
            float a2Bottom = animal2Bottom + animal2Yoffset;

            bool animal1InOutput = (a1Bottom < -0.75f);
            bool animal2InOutput = (a2Bottom < -0.75f);

            bool clickedAnimal1 =
                animal1InOutput &&
                ndcX >= a1Left && ndcX <= a1Right &&
                ndcY >= a1Bottom && ndcY <= a1Top;

            bool clickedAnimal2 =
                animal2InOutput &&
                ndcX >= a2Left && ndcX <= a2Right &&
                ndcY >= a2Bottom && ndcY <= a2Top;

            if (clickedAnimal1 || clickedAnimal2) {
                resetGame();
                continue;
            }
        }



        if (gameActive) {
            bool movedLeft = glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS;
            bool movedRight = glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS;

            if (movedLeft) {
                clawX -= clawSpeed;
                if (grabbed) {
                    if (grabbedAnimal == 1) animal1Xoffset -= clawSpeed;
                    if (grabbedAnimal == 2) animal2Xoffset -= clawSpeed;
                }
            }

            if (movedRight) {
                clawX += clawSpeed;
                if (grabbed) {
                    if (grabbedAnimal == 1) animal1Xoffset += clawSpeed;
                    if (grabbedAnimal == 2) animal2Xoffset += clawSpeed;
                }
            }

            if (grabbed) {
                float limitLeft = leftLimit;
                float limitRight = rightLimit;

                if (grabbedAnimal == 1) {
                    float curCenter = (animal1Left + animal1Right) * 0.5f + animal1Xoffset;
                    if (curCenter < limitLeft) animal1Xoffset += (limitLeft - curCenter);
                    if (curCenter > limitRight) animal1Xoffset -= (curCenter - limitRight);
                }

                if (grabbedAnimal == 2) {
                    float curCenter = (animal2Left + animal2Right) * 0.5f + animal2Xoffset;
                    if (curCenter < limitLeft) animal2Xoffset += (limitLeft - curCenter);
                    if (curCenter > limitRight) animal2Xoffset -= (curCenter - limitRight);
                }
            }
        }

        if (clawX < leftLimit)  clawX = leftLimit;
        if (clawX > rightLimit) clawX = rightLimit;

        float clawOffset = clawX;

        float cableMoved[] = {
            -0.01f + clawOffset, 0.60f, 0,0,
            -0.01f + clawOffset, 0.48f, 0,0,
             0.01f + clawOffset, 0.48f, 0,0,
             0.01f + clawOffset, 0.60f, 0,0
        };

        float clawMoved[] = {
            -0.06f + clawOffset, 0.48f, 0,1,
            -0.06f + clawOffset, 0.36f, 0,0,
             0.06f + clawOffset, 0.36f, 1,0,
             0.06f + clawOffset, 0.48f, 1,1
        };

        glBindBuffer(GL_ARRAY_BUFFER, VBOcable);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cableMoved), cableMoved);

        glBindBuffer(GL_ARRAY_BUFFER, VBOclaw);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(clawMoved), clawMoved);

        bool sPressed = glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS;

        if (gameActive && sPressed && !animalFalling) {
            if (!grabbed && !clawDescending && !clawAscending)
                clawDescending = true;
            else if (grabbed && !clawDescending && !clawAscending) {
                grabbed = false;
                animalFalling = true;
            }
        }

        if (animalFalling) {
            float fallSpeed = 0.015f;

            float* yOffset = (grabbedAnimal == 1) ? &animal1Yoffset : &animal2Yoffset;
            float* xOffset = (grabbedAnimal == 1) ? &animal1Xoffset : &animal2Xoffset;

            float baseBottom = (grabbedAnimal == 1) ? animal1Bottom : animal2Bottom;
            float baseRight = (grabbedAnimal == 1) ? animal1Right : animal2Right;

            *yOffset -= fallSpeed;

            float animalBottomY = baseBottom + *yOffset;
            float animalRightX = baseRight + *xOffset;

            bool overHoleX = (animalRightX >= holeLeft && animalRightX <= holeRight);
            bool atHoleHeight = (animalBottomY <= holeTopY);

            float boxBottom = -0.5f;

            if (overHoleX && atHoleHeight)
            {
                animalFalling = false;
                fallingToOutput = false;

                float baseLeft = (grabbedAnimal == 1 ? animal1Left : animal2Left);
                float baseRight = (grabbedAnimal == 1 ? animal1Right : animal2Right);
                float baseBottom = (grabbedAnimal == 1 ? animal1Bottom : animal2Bottom);

                float appearCenterX = (0.36f + 0.90f) * 0.5f;   // centar donje zone
                float appearBottomY = -0.83f;                  // malo iznad dna

                float newXoffset = appearCenterX - (baseLeft + baseRight) * 0.5f;
                float newYoffset = appearBottomY - baseBottom;

                if (grabbedAnimal == 1) {
                    animal1Xoffset = newXoffset;
                    animal1Yoffset = newYoffset;
                }
                else {
                    animal2Xoffset = newXoffset;
                    animal2Yoffset = newYoffset;
                }

                clawLocked = true;
                lampBlinking = true;
                lampTimer = glfwGetTime();
            }

            else if (animalBottomY <= boxBottom) {
                animalFalling = false;
                grabbed = false;
                grabbedAnimal = -1;
                clawDescending = false;
                clawAscending = false;
            }
        }

        if (fallingToOutput) {
            float fallSpeed2 = 0.02f;

            float* yOffset = (grabbedAnimal == 1) ? &animal1Yoffset : &animal2Yoffset;
            float baseBottom = (grabbedAnimal == 1) ? animal1Bottom : animal2Bottom;

            *yOffset -= fallSpeed2;
            float animalBottomY = baseBottom + *yOffset;

            if (animalBottomY <= outputFloorY) {
                fallingToOutput = false;
                grabbed = false;
                grabbedAnimal = -1;
                clawLocked = true;
                lampBlinking = true;
                lampTimer = glfwGetTime();
            }
        }

        float clawBottom = clawY - 0.12f;

        float a1Left = animal1Left + animal1Xoffset;
        float a1Right = animal1Right + animal1Xoffset;
        float a1Top = animal1Top + animal1Yoffset;
        float a1Bottom = animal1Bottom + animal1Yoffset;

        float a2Left = animal2Left + animal2Xoffset;
        float a2Right = animal2Right + animal2Xoffset;
        float a2Top = animal2Top + animal2Yoffset;
        float a2Bottom = animal2Bottom + animal2Yoffset;

        bool hitAnimal1 =
            gameActive &&
            !clawLocked &&
            clawDescending &&
            (clawBottom <= a1Top) &&
            (clawX >= a1Left && clawX <= a1Right);

        bool hitAnimal2 =
            gameActive &&
            !clawLocked &&
            clawDescending &&
            (clawBottom <= a2Top) &&
            (clawX >= a2Left && clawX <= a2Right);

        if (hitAnimal1) {
            grabbed = true;
            grabbedAnimal = 1;
            clawDescending = false;
            clawAscending = true;
        }

        if (hitAnimal2) {
            grabbed = true;
            grabbedAnimal = 2;
            clawDescending = false;
            clawAscending = true;
        }

        if (clawDescending) {
            clawY -= clawDropSpeed;
            if (clawY <= clawBottomLimit) {
                clawY = clawBottomLimit;
                clawDescending = false;
                clawAscending = true;
            }
        }
        else if (clawAscending) {
            clawY += clawDropSpeed;
            if (grabbed) {
                if (grabbedAnimal == 1) animal1Yoffset += clawDropSpeed;
                else animal2Yoffset += clawDropSpeed;
            }
            if (clawY >= 0.48f) {
                clawY = 0.48f;
                clawAscending = false;
            }
        }

        float cableMovedVertical[] = {
            -0.01f + clawOffset, cableTopY, 0,0,
            -0.01f + clawOffset, clawY,     0,0,
             0.01f + clawOffset, clawY,     0,0,
             0.01f + clawOffset, cableTopY, 0,0
        };

        float clawMovedVertical[] = {
            -0.06f + clawOffset, clawY,      0,1,
            -0.06f + clawOffset, clawY - 0.12f,0,0,
             0.06f + clawOffset, clawY - 0.12f,1,0,
             0.06f + clawOffset, clawY,      1,1
        };

        glBindBuffer(GL_ARRAY_BUFFER, VBOcable);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(cableMovedVertical), cableMovedVertical);

        glBindBuffer(GL_ARRAY_BUFFER, VBOclaw);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(clawMovedVertical), clawMovedVertical);

        /* RENDER  */

        // 1) BOX
        glUseProgram(boxShader);
        glUniform4f(glGetUniformLocation(boxShader, "uColor"), 1, 1, 1, 0.2f);
        glBindVertexArray(VAObox);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        

        // 2) ANIMALS
        glUseProgram(animalShader);
        glActiveTexture(GL_TEXTURE0);

        float animal1Moved[] = {
            animal1Left + animal1Xoffset, animal1Top + animal1Yoffset, 0,1,
            animal1Left + animal1Xoffset, animal1Bottom + animal1Yoffset, 0,0,
            animal1Right + animal1Xoffset, animal1Bottom + animal1Yoffset, 1,0,
            animal1Right + animal1Xoffset, animal1Top + animal1Yoffset, 1,1
        };

        float animal2Moved[] = {
            animal2Left + animal2Xoffset, animal2Top + animal2Yoffset, 0,1,
            animal2Left + animal2Xoffset, animal2Bottom + animal2Yoffset, 0,0,
            animal2Right + animal2Xoffset, animal2Bottom + animal2Yoffset, 1,0,
            animal2Right + animal2Xoffset, animal2Top + animal2Yoffset, 1,1
        };


        glBindBuffer(GL_ARRAY_BUFFER, VBOanimal1);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(animal1Moved), animal1Moved);
        glBindTexture(GL_TEXTURE_2D, texAnimal1);
        glBindVertexArray(VAOanimal1);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        glBindBuffer(GL_ARRAY_BUFFER, VBOanimal2);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(animal2Moved), animal2Moved);
        glBindTexture(GL_TEXTURE_2D, texAnimal2);
        glBindVertexArray(VAOanimal2);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 3) PLATFORM
        glUseProgram(boxShader);
        glUniform4f(glGetUniformLocation(boxShader, "uColor"), 0.9f, 0.2f, 0.2f, 0.2f);
        glBindVertexArray(VAOplatform);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        // 4) OUTPUT ZONA
        glUseProgram(boxShader);
        glUniform4f(glGetUniformLocation(boxShader, "uColor"), 1, 1, 1, 0.2f);
        glBindVertexArray(VAOoutput);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        // 5) DROPSLOT (rupa)
        glUseProgram(boxShader);
        glUniform4f(glGetUniformLocation(boxShader, "uColor"), 0.15f, 0.15f, 0.15f, 1);
        glBindVertexArray(VAOdropslot);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 6) SLOT ZA ŽETON
        glUseProgram(boxShader);
        glUniform4f(glGetUniformLocation(boxShader, "uColor"), 0.05f, 0.05f, 0.05f, 1);
        glBindVertexArray(VAOslot);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 7) LAMPICA
        glUseProgram(boxShader);
        if (lampBlinking) {
            double t = glfwGetTime();
            double dt = t - lampTimer;
            bool green = ((int)(dt / 0.5) % 2 == 0);
            if (green)
                glUniform4f(glGetUniformLocation(boxShader, "uColor"), 0, 1, 0, 1);
            else
                glUniform4f(glGetUniformLocation(boxShader, "uColor"), 1, 0, 0, 1);
        }
        else if (gameActive)
            glUniform4f(glGetUniformLocation(boxShader, "uColor"), 0.2f, 0.4f, 1, 1);
        else
            glUniform4f(glGetUniformLocation(boxShader, "uColor"), 0.7f, 0.7f, 0.7f, 1);

        glBindVertexArray(VAOlamp);
        glDrawArrays(GL_TRIANGLE_FAN, 0, NUM_SEGMENTS + 2);

        // 8) CABLE
        glUseProgram(boxShader);
        glUniform4f(glGetUniformLocation(boxShader, "uColor"), 0.6f, 0.6f, 0.6f, 1);
        glBindVertexArray(VAOcable);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 9) CLAW
        glUseProgram(animalShader);
        glActiveTexture(GL_TEXTURE0);
        if (!gameActive || clawLocked || grabbed)
            glBindTexture(GL_TEXTURE_2D, texClawClosed);
        else
            glBindTexture(GL_TEXTURE_2D, texClawOpen);
        glBindVertexArray(VAOclaw);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

        // 10) WATERMARK
        drawWatermark(watermarkShader, VAOwatermark);

        glfwSwapBuffers(window);
        glfwPollEvents();

        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) break;

        while (glfwGetTime() - initFrameTime < 1 / 75.0) {}
    }


    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}