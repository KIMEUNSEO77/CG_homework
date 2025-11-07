#include <glew.h>
#include <freeglut.h>
#include <freeglut_ext.h> 
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "filetobuf.h"
#include "shaderMaker.h"
#include "obj_load.h"
#include <random>
#include <vector>

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);

Mesh gCube;
int cubeCount_x = 0;    // 가로 개수
int cubeCount_z = 0;    // 세로 개수

bool animationActive = true; float currentY = -10.0f;
bool orthoMode = false;  // 직각 투영 모드

struct Cube
{
	glm::vec3 position;
	glm::vec3 color;
	float height;
};
std::vector<std::vector<Cube>> cubes;


float randomFloat(float a, float b)
{
	std::random_device rd;
	std::mt19937 gen(rd());  // Mersenne Twister 엔진
	std::uniform_real_distribution<float> dist(a, b);

	return dist(gen);
}

// 사용자에게 정육면체의 개수 입력 받기
void InputCubeCount()
{
	std::cout << "가로, 세로 개수 5~25 입력: ";
	std::cin >> cubeCount_x >> cubeCount_z;
	std::cout << "가로 개수: " << cubeCount_x << ", 세로 개수: " << cubeCount_z << std::endl;
	
	if (cubeCount_x < 5 || cubeCount_x > 25)
	{
		std::cerr << "가로 개수 잘못 입력" << std::endl;
		InputCubeCount();
	}

	if (cubeCount_z < 5 || cubeCount_z > 25)
	{
		std::cerr << "세로 개수 잘못 입력" << std::endl;
		InputCubeCount();
	}

	cubes.resize(cubeCount_x);
	for (int i = 0; i < cubeCount_x; ++i) 
		cubes[i].resize(cubeCount_z);
	
	// 큐브들 정보 초기화
	for (int i = 0; i < cubeCount_x; ++i)
	{
		for (int j = 0; j < cubeCount_z; ++j)
		{
			cubes[i][j].position = glm::vec3(-2.0f + i, -5.0f, -3.0f + j);
			cubes[i][j].color = glm::vec3(randomFloat(0.1f, 1.0f),
				randomFloat(0.1f, 1.0f), randomFloat(0.1f, 1.0f));
			cubes[i][j].height = randomFloat(9.0f, 14.0f);
		}
	}
}

// 콘솔창에 명령어들 출력
void PrintInstructions()
{
	std::cout << "<<키보드 명령어들>>\n";
	std::cout << "q: 종료\n";
}

void Timer(int value)
{
	if (animationActive)
	{
		currentY += 0.1f;
		if (currentY >= -3.0f)
		{
			currentY = -3.0f;
			animationActive = false;
		}
		glutPostRedisplay();
		glutTimerFunc(16, Timer, 0); // 약 60 FPS
	}
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'o': orthoMode = true; glutPostRedisplay(); break;
	case 'p': orthoMode = false; glutPostRedisplay(); break;
	case 'q': exit(0); break;
	}
}

int main(int argc, char** argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);  // 깊이 버퍼 추가
	glutInitWindowPosition(100, 100);
	glutInitWindowSize(width, height);
	glutCreateWindow("2025_ComputerGraphics_Homework");

	glutDisplayFunc(drawScene);
	glutReshapeFunc(Reshape);

	glewExperimental = GL_TRUE;
	glewInit();

	glEnable(GL_DEPTH_TEST); // 깊이 테스트 활성화
	// 은면 제거 생략 일단은
	InputCubeCount();   // 큐브 개수 입력 받기

	make_vertexShaders();
	make_fragmentShaders();
	shaderProgramID = make_shaderProgram();

	if (!LoadOBJ_PosNorm_Interleaved("unit_cube.obj", gCube)) 
	{
		std::cerr << "Failed to load unit_cube.obj\n";
		return 1;
	}

	PrintInstructions();  // 콘솔창에 명령어들 출력

	glutKeyboardFunc(Keyboard);
	glutTimerFunc(0, Timer, 0); // 타이머 시작
	glutMainLoop();

	return 0;
}

// 큐브 그리는 함수
void DrawCube(const Mesh& mesh, GLuint shaderProgram, const glm::mat4& model, const glm::vec3& color)
{
	GLint modelLoc = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

	GLint colorLoc = glGetUniformLocation(shaderProgram, "vColor");
	glUniform3fv(colorLoc, 1, &color[0]);

	glBindVertexArray(mesh.vao);
	glDrawArrays(GL_TRIANGLES, 0, mesh.count);
	glBindVertexArray(0);
}

GLvoid drawScene()
{
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramID);

	GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
	GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");

	float centerX = (cubeCount_x - 1) / 2.0f;
	float centerZ = (cubeCount_z - 1) / 2.0f;

	glm::vec3 cameraPos = glm::vec3(centerX, 0.0f, 15.0f + cubeCount_z);
	glm::vec3 cameraDirection = glm::vec3(centerX, 0.0f, centerZ);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 vTransform = glm::mat4(1.0f);
	vTransform = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &vTransform[0][0]);

	glm::mat4 pTransform = glm::mat4(1.0f);
	if (orthoMode)
		pTransform = glm::ortho(-centerX, centerX, -3.0f, 3.0f, 0.1f, 100.0f);
	else
		pTransform = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

	glUniformMatrix4fv(projLoc, 1, GL_FALSE, &pTransform[0][0]);

	// 바닥
	glm::mat4 ground = glm::mat4(1.0f);
	ground = glm::translate(ground, glm::vec3(0.0f, -0.5f, 0.0f));
	ground = glm::rotate(ground, glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	ground = glm::scale(ground, glm::vec3(100.0f, 0.05f, 100.0f));
	DrawCube(gCube, shaderProgramID, ground, glm::vec3(0.0f, 0.0f, 0.0f));

	// 큐브들
	for (int i = 0; i < cubeCount_x; ++i)
	{
		for (int j = 0; j < cubeCount_z; j++)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(cubes[i][j].position.x,
				currentY, cubes[i][j].position.z));
			model = glm::rotate(model, glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::scale(model, glm::vec3(1.0f, cubes[i][j].height, 1.0f));
			DrawCube(gCube, shaderProgramID, model, cubes[i][j].color);
		}
	}

	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, w, h);
}