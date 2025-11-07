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

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);

Mesh gCube;
int cubeCount_x = 0;    // 가로 개수
int cubeCount_z = 0;    // 세로 개수

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
}

// 콘솔창에 명령어들 출력
void PrintInstructions()
{
	std::cout << "<<키보드 명령어들>>\n";
	std::cout << "q: 종료\n";
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
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

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 15.0f);
	glm::vec3 cameraDirection = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

	glm::mat4 vTransform = glm::mat4(1.0f);
	vTransform = glm::lookAt(cameraPos, cameraDirection, cameraUp);
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &vTransform[0][0]);

	glm::mat4 pTransform = glm::mat4(1.0f);
	pTransform = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, &pTransform[0][0]);

	glm::mat4 test = glm::mat4(1.0f);
	test = glm::translate(test, glm::vec3(0.0f, 0.0f, -3.0f));
	DrawCube(gCube, shaderProgramID, test, glm::vec3(0.0f, 0.8f, 0.8f));

	
	glm::mat4 ground = glm::mat4(1.0f);
	ground = glm::translate(ground, glm::vec3(0.0f, -0.5f, 0.0f));
	ground = glm::rotate(ground, glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	ground = glm::scale(ground, glm::vec3(100.0f, 0.05f, 100.0f));
	DrawCube(gCube, shaderProgramID, ground, glm::vec3(0.0f, 0.0f, 0.0f));
	
	glm::vec3 position;
	glm::vec3 color;

	for (int i = 0; i < cubeCount_x; ++i)
	{
		for (int j = 0; j < cubeCount_z; ++j)
		{		
		    position = glm::vec3(-2.0f + i, -0.5f, -3.0f + j);
		    color = glm::vec3(randomFloat(0.1f, 1.0f), randomFloat(0.1f, 1.0f), randomFloat(0.1f, 1.0f));
		    
		    glm::mat4 model = glm::mat4(1.0f);
		    model = glm::translate(model, position);
		    model = glm::rotate(model, glm::radians(15.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		    model = glm::scale(model, glm::vec3(1.0f, randomFloat(4.0f, 10.0f), 1.0f));
		    DrawCube(gCube, shaderProgramID, model, color);
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