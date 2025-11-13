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

// 미로
#include <algorithm>
#include <stack>
#include <utility>

void make_vertexShaders();
void make_fragmentShaders();
GLuint make_shaderProgram();
GLvoid drawScene();
GLvoid Reshape(int w, int h);

Mesh gCube;
int cubeCount_x = 0;    // 가로 개수
int cubeCount_z = 0;    // 세로 개수

bool animationActive = true; float currentY = -3.0f;
bool orthoMode = false;  // 직각 투영 모드
float moveCubeZ = 0.0f;      // z축 이동
bool updownAnimation = false;   // 위아래 애니메이션
bool rotatingYPlus = false;    // Y축 중심 양의 방향 회전
bool rotatingYMinus = false;  // y축 중심 음의 방향 회전
float angleCameraY = 0.0f; // 카메라 Y축 회전 각도

// 미로
bool mazeMode = false;   // 미로 모드
bool lowMode = false;    // 낮은 벽 모드(낮아지고, 움직임이 멈춰야 함)
bool playerActive = false; // 플레이어 있는지 없는지

// 로봇
float moveX = 0.0f; float moveZ = 0.0f; float moveSpeed = 0.05f;
float angleY = 0.0f;  // 움직일 때 방향 (로봇 전체 방향)
float angleArm_X = 0.0f;   //  팔 각도
int dir = 1;
float angleLeg_X = 0.0f;   //  다리 각도

float limitAngleX = 45.0f; float limitAngleY = 10.0f;
bool cameraPOV = false; // 카메라 시점 모드(true: 로봇 1인칭 시점. false: 기본 3인칭 시점)

struct Cube
{
	glm::vec3 position;
	glm::vec3 color;
	float height;
	float currentY;  // 현재 y 위치
	bool goingUp;    // 위로 가는 중인지
	float moveingSpeed;  // 움직이는 속도

	bool active = true; // 미로 모드에서 그릴지 말지 (그리면 벽)
};
std::vector<std::vector<Cube>> cubes;

static std::mt19937 rng(std::random_device{}());

void GenerateMaze()
{
	// 짝수면 하나 줄여서 맞춤
	if (cubeCount_x % 2 == 0) cubeCount_x -= 1;
	if (cubeCount_z % 2 == 0) cubeCount_z -= 1;

	// 모든 칸을 초기화
	for (int i = 0; i < cubeCount_x; ++i)
		for (int j = 0; j < cubeCount_z; ++j)
			cubes[i][j].active = true;

	// 방문표시
	std::vector<std::vector<bool>> vis(cubeCount_x, std::vector<bool>(cubeCount_z, false));

	auto in = [&](int x, int z) 
		{
		return (x > 0 && x < cubeCount_x - 1 && z > 0 && z < cubeCount_z - 1);
		};

	// 시작 셀(홀수,홀수) 선택
	int sx = 1, sz = 1;  // 고정해도 되고 랜덤해도 됨
	vis[sx][sz] = true;
	cubes[sx][sz].active = false; // 길

	std::stack<std::pair<int, int>> st;
	st.push({ sx, sz });

	// 이웃 방향(2칸씩 이동): 좌우상하
	const int dx[4] = { +2, -2, 0, 0 };
	const int dz[4] = { 0, 0, +2, -2 };

	while (!st.empty())
	{
		auto cur = st.top();
		int x = cur.first;
		int z = cur.second;

		// 방문하지 않은 이웃(2칸 떨어진 홀수칸) 수집
		std::vector<int> cand;
		for (int k = 0; k < 4; ++k)
		{
			int nx = x + dx[k];
			int nz = z + dz[k];
			if (in(nx, nz) && !vis[nx][nz] && (nx % 2 == 1) && (nz % 2 == 1))
				cand.push_back(k);
		}

		if (cand.empty())
		{
			st.pop();
			continue;
		}

		// 랜덤 이웃 하나 선택
		std::shuffle(cand.begin(), cand.end(), rng);
		int k = cand.front();
		int nx = x + dx[k];
		int nz = z + dz[k];

		// 현재(x,z)와 (nx,nz) 사이의 벽(1칸 차이)을 뚫음
		int wx = x + dx[k] / 2;
		int wz = z + dz[k] / 2;

		vis[nx][nz] = true;
		cubes[nx][nz].active = false; // 길
		cubes[wx][wz].active = false; // 벽 제거(길)

		st.push({ nx, nz });
	}

	// 입구/출구(테두리 구멍) 만들어 주기(원하면)
	cubes[1][0].active = false;                           // 입구
	cubes[cubeCount_x - 2][cubeCount_z - 1].active = false;   // 출구
}

float randomFloat(float a, float b)
{
	std::random_device rd;
	std::mt19937 gen(rd());  // Mersenne Twister 엔진
	std::uniform_real_distribution<float> dist(a, b);

	return dist(gen);
}

// 큐브들 정보 초기화
void InitCube()
{
	cubes.resize(cubeCount_x);
	for (int i = 0; i < cubeCount_x; ++i)
		cubes[i].resize(cubeCount_z);

	for (int i = 0; i < cubeCount_x; ++i)
	{
		for (int j = 0; j < cubeCount_z; ++j)
		{
			cubes[i][j].position = glm::vec3(-2.0f + i, 0.0f, -3.0f + j);
			cubes[i][j].color = glm::vec3(randomFloat(0.1f, 1.0f),
				randomFloat(0.1f, 1.0f), randomFloat(0.1f, 1.0f));
			cubes[i][j].height = randomFloat(9.0f, 17.0f);
			cubes[i][j].currentY = -10.0f;
			cubes[i][j].goingUp = rand() % 2;           // 방향 랜덤
			cubes[i][j].moveingSpeed = randomFloat(0.01f, 0.3f); // 속도 랜덤
		}
	}
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
	InitCube();
}

void SpeedChange(float delta)
{
	for (int i = 0; i < cubeCount_x; ++i)
	{
		for (int j = 0; j < cubeCount_z; ++j)
		{
			cubes[i][j].moveingSpeed += delta;
			if (cubes[i][j].moveingSpeed <= 0.002f)
				cubes[i][j].moveingSpeed = 0.002f;
			if (cubes[i][j].moveingSpeed >= 0.5f)
				cubes[i][j].moveingSpeed = 0.5f;
		}
	}
}

void LowMode()
{
	for (int i = 0; i < cubeCount_x; ++i)
	{
		for (int j = 0; j < cubeCount_z; ++j)
		{
			if (!cubes[i][j].active) continue;  // 애니메이션 x
			cubes[i][j].currentY = -0.5f;
			cubes[i][j].height = 5.0f;
		}
	}
}

// 콘솔창에 명령어들 출력
void PrintInstructions()
{
	std::cout << "<<키보드 명령어들>>\n";
	std::cout << "o: 직각 투영 모드\n";
	std::cout << "p: 원근 투영 모드\n";
	std::cout << "z: z축 양의 방향 이동\n";
	std::cout << "Z: z축 음의 방향 이동\n";
	std::cout << "m: 큐브들이 위아래로 움직임 시작\n";
	std::cout << "M: 큐브들이 위아래로 움직임 정지\n";
	std::cout << "y: 카메라 Y축 중심 양의 방향 회전 시작/정지\n";
	std::cout << "Y: 카메라 Y축 중심 음의 방향 회전 시작/정지\n";
	std::cout << "+: 큐브 움직임 속도 증가\n";
	std::cout << "-: 큐브 움직임 속도 감소\n";
	std::cout << "r: 미로 제작(누를 때마다 새로운 미로가 제작됨)\n";
	std::cout << "v: 낮은 벽 모드 시작, 움직임 시작/정지\n";
	std::cout << "s: 플레이어(로봇) 등장\n";
	std::cout << "1: 로봇 1인칭 시점 카메라 모드\n";
	std::cout << "3: 기본 3인칭 시점 카메라 모드\n";
	std::cout << "q: 종료\n";
}

void MoveArmX()
{
	if (angleArm_X > limitAngleX) dir = -1;
	else if (angleArm_X < -limitAngleX) dir = 1;
	angleArm_X += dir * 2.0f;

	if (angleLeg_X > limitAngleY) dir = -1;
	else if (angleLeg_X < -limitAngleY) dir = 1;
	angleLeg_X += dir * 1.0f;
}


void MoveX(float speed)
{
	moveX += speed;
	glutPostRedisplay();
}

void MoveZ(float speed)
{
	moveZ += speed;
	glutPostRedisplay();
}

void IncreaseSpeed(float delta)
{
	moveSpeed += delta;
	if (moveSpeed < 0.01f) moveSpeed = 0.01f;
	if (moveSpeed > 0.2f) moveSpeed = 0.2f;

	if (delta > 0)
	{
		if (limitAngleX < 60.0f)
			limitAngleX += 3.0f;
		if (limitAngleY < 20.0f)
			limitAngleY += 1.0f;
	}
	else
	{
		if (limitAngleX > 15.0f)
			limitAngleX -= 3.0f;
		if (limitAngleY > 5.0f)
			limitAngleY -= 1.0f;
	}
}

void Timer(int value)
{
	if (animationActive)
	{
		for (int i = 0; i < cubeCount_x; ++i)
		{
			for (int j = 0; j < cubeCount_z; ++j)
			{
				if (!cubes[i][j].active) continue;  // 애니메이션 x

				cubes[i][j].currentY += 0.1f;

				if (cubes[i][j].currentY >= -3.0f)
				{
					cubes[i][j].currentY = -3.0f;
					animationActive = false;
				}					
			}
		}
	}

	if (updownAnimation)
	{
		for (int i = 0; i < cubeCount_x; ++i)
		{
			for (int j = 0; j < cubeCount_z; ++j)
			{
				if (!cubes[i][j].active) continue;  // 애니메이션 x

				if (cubes[i][j].goingUp)
				{
					cubes[i][j].currentY += cubes[i][j].moveingSpeed;
					if (cubes[i][j].currentY >= -1.5f)
						cubes[i][j].goingUp = false;
				}
				else
				{
					cubes[i][j].currentY -= cubes[i][j].moveingSpeed;
					if (cubes[i][j].currentY <= -5.0f)
						cubes[i][j].goingUp = true;
				}
			}
		}
	}

	if (rotatingYPlus) angleCameraY += 0.2f;
	if (rotatingYMinus) angleCameraY -= 0.2f;

	glutPostRedisplay();
	glutTimerFunc(16, Timer, 0); // 약 60 FPS
}

GLvoid Keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 'o': orthoMode = true; glutPostRedisplay(); break;
	case 'p': orthoMode = false; glutPostRedisplay(); break;
	case 'z': if (!orthoMode) moveCubeZ += 1.0f; glutPostRedisplay(); break;
	case 'Z': if (!orthoMode) moveCubeZ -= 1.0f; glutPostRedisplay(); break;
	case 'm': updownAnimation = true; break;
	case 'M': updownAnimation = false; break;
	case '+': SpeedChange(0.01f); break;
	case '-': SpeedChange(-0.01f); break;
	case 'y': rotatingYPlus = !rotatingYPlus; rotatingYMinus = false; break;
	case 'Y': rotatingYMinus = !rotatingYMinus; rotatingYPlus = false; break;
	case 'r': mazeMode = true; GenerateMaze(); glutPostRedisplay(); break;
	case 'v': updownAnimation = !updownAnimation; LowMode(); glutPostRedisplay(); break;
	case 's': playerActive = true; glutPostRedisplay(); break;
	case '1': cameraPOV = true; glutPostRedisplay(); break;
	case '3': cameraPOV = false; glutPostRedisplay(); break;
	case 'q': exit(0); break;
	}
}

void SpecialKeyboard(int key, int x, int y)
{
	switch (key)
	{
	case GLUT_KEY_UP: MoveZ(-moveSpeed); angleY = 180.0f; MoveArmX();  break;
	case GLUT_KEY_DOWN: MoveZ(moveSpeed); angleY = 0.0f; MoveArmX(); break;
	case GLUT_KEY_LEFT: MoveX(-moveSpeed); angleY = -90.0f; MoveArmX(); break;
	case GLUT_KEY_RIGHT: MoveX(moveSpeed);  angleY = 90.0f; MoveArmX(); break;
	}
	glutPostRedisplay();
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
	glutSpecialFunc(SpecialKeyboard);
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
	glViewport(0, 0, width, height);
	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glUseProgram(shaderProgramID);

	GLint viewLoc = glGetUniformLocation(shaderProgramID, "view");
	GLint projLoc = glGetUniformLocation(shaderProgramID, "projection");

	// drawScene() 함수 내 카메라 설정 부분을 수정
	glm::vec3 cameraPos, cameraDirection, cameraUp;
	float centerX = (cubeCount_x - 1) / 2.0f;
	float centerZ = (cubeCount_z - 1) / 2.0f;

	if (cameraPOV && playerActive) 
	{
		// angleY(도)를 라디안으로 변환
		float rad = glm::radians(angleY);
		constexpr float pitch = glm::radians(-5.0f);

		// 로봇이 바라보는 방향 벡터 계산 (y축은 고정, z축 기준)
		glm::vec3 forward = glm::vec3(
			sin(rad) * cos(pitch), // x
			sin(pitch),            // y
			cos(rad) * cos(pitch)  // z
		);

		// 1인칭 시점: 로봇 머리 위치
		glm::vec3 robotPos = glm::vec3(moveX, 1.5f, moveZ); // 시점을 약간 뒤로
		cameraPos = robotPos;
		cameraDirection = robotPos + forward;

		cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

		glm::mat4 vTransform = glm::mat4(1.0f);
		//vTransform = glm::lookAt(cameraPos, forward, cameraUp);
		vTransform = glm::lookAt(cameraPos, cameraDirection, cameraUp);
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &vTransform[0][0]);
	}
	else 
	{
		// 기존 3인칭 시점
		cameraPos = glm::vec3(centerX, 0.0f, 15.0f + cubeCount_z);
		cameraDirection = glm::vec3(centerX, 0.0f, centerZ);
		cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);

		glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(angleCameraY), glm::vec3(0.0f, 1.0f, 0.0f))
			* glm::rotate(glm::mat4(1.0f), glm::radians(-15.0f), glm::vec3(1.0f, 0.0f, 0.0f));
		cameraPos = glm::vec3(rotation * glm::vec4(cameraPos - cameraDirection, 1.0f)) + cameraDirection;

		glm::mat4 vTransform = glm::mat4(1.0f);
		vTransform = glm::lookAt(cameraPos, cameraDirection, cameraUp);
		glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &vTransform[0][0]);
	}

	glm::mat4 pTransform = glm::mat4(1.0f);
	if (orthoMode)
	{
		float aspect = (float)width / (float)height;
		float margin = 1.0f;
		float maxH = 14.0f;
		float halfContentX = 0.5f * cubeCount_x + margin;
		float halfContentY = 0.5f * maxH + margin;

		// 더 큰 쪽에 맞춰 확장
		float halfY = std::max(halfContentY, halfContentX / aspect);
		float halfX = halfY * aspect;

		pTransform = glm::ortho(-halfX, halfX, -halfY, halfY, 0.1f, 1000.0f);
	}
	else
		pTransform = glm::perspective(glm::radians(45.0f), (float)width / (float)height, 0.1f, 100.0f);

	glUniformMatrix4fv(projLoc, 1, GL_FALSE, &pTransform[0][0]);

	// 바닥
	glm::mat4 ground = glm::mat4(1.0f);
	ground = glm::translate(ground, glm::vec3(0.0f, -0.5f, 0.0f));
	ground = glm::scale(ground, glm::vec3(100.0f, 0.05f, 100.0f));
	DrawCube(gCube, shaderProgramID, ground, glm::vec3(0.0f, 0.0f, 0.0f));

	// 큐브들
	for (int i = 0; i < cubeCount_x; ++i)
	{
		for (int j = 0; j < cubeCount_z; j++)
		{
			if (!cubes[i][j].active) continue;  // 길은 그리지 않음

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(cubes[i][j].position.x,
				cubes[i][j].currentY, cubes[i][j].position.z + moveCubeZ));
			model = glm::scale(model, glm::vec3(1.0f, cubes[i][j].height, 1.0f));
			DrawCube(gCube, shaderProgramID, model, cubes[i][j].color);
		}
	}

	if (playerActive)
	{
		// 큐브 그리기
		// 공통
		glm::mat4 share = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		share = glm::translate(share, glm::vec3(0.0f, -1.5f + cubeCount_z * 0.05f, -5.0f));
		share = glm::scale(share, glm::vec3(0.5f, 0.5f, 0.5f));

		// 로봇 그리기
		glm::mat4 robotBase = share;
		robotBase = glm::translate(robotBase, glm::vec3(moveX, 5.0f, moveZ));
		robotBase = glm::rotate(robotBase, glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
		// 머리
		glm::mat4 robotHead = robotBase;
		robotHead = glm::translate(robotHead, glm::vec3(0.0f, 0.0f, 0.0f));
		robotHead = glm::scale(robotHead, glm::vec3(1.0f, 1.0f, 1.0f));
		DrawCube(gCube, shaderProgramID, robotHead, glm::vec3(1.0f, 0.7f, 0.3f));

		// 코
		glm::mat4 robotNose = robotBase;
		robotNose = glm::translate(robotNose, glm::vec3(0.0f, 0.0f, 0.5f));
		robotNose = glm::scale(robotNose, glm::vec3(0.2f, 0.2f, 0.3f));
		DrawCube(gCube, shaderProgramID, robotNose, glm::vec3(1.0f, 0.0f, 0.0f));

		// 몸통
		glm::mat4 robotBody = robotBase;
		robotBody = glm::translate(robotBody, glm::vec3(0.0f, -1.0f, 0.0f));
		robotBody = glm::scale(robotBody, glm::vec3(1.0f, 1.5f, 1.0f));
		DrawCube(gCube, shaderProgramID, robotBody, glm::vec3(0.5f, 0.9f, 0.5f));

		// 왼팔
		glm::mat4 robotArmL = robotBase;
		robotArmL = glm::rotate(robotArmL, glm::radians(angleArm_X), glm::vec3(1.0f, 0.0f, 0.0f));
		robotArmL = glm::translate(robotArmL, glm::vec3(-0.5f, -1.0f, 0.0f));
		robotArmL = glm::scale(robotArmL, glm::vec3(0.3f, 1.2f, 0.3f));
		DrawCube(gCube, shaderProgramID, robotArmL, glm::vec3(0.7f, 0.6f, 0.7f));
		// 오른팔
		glm::mat4 robotArmR = robotBase;
		robotArmR = glm::rotate(robotArmR, glm::radians(-angleArm_X), glm::vec3(1.0f, 0.0f, 0.0f));
		robotArmR = glm::translate(robotArmR, glm::vec3(0.5f, -1.0f, 0.0f));
		robotArmR = glm::scale(robotArmR, glm::vec3(0.3f, 1.2f, 0.3f));
		DrawCube(gCube, shaderProgramID, robotArmR, glm::vec3(0.3f, 0.4f, 0.3f));

		// 왼다리
		glm::mat4 robotLegL = robotBase;
		robotLegL = glm::rotate(robotLegL, glm::radians(-angleLeg_X), glm::vec3(1.0f, 0.0f, 0.0f));
		robotLegL = glm::translate(robotLegL, glm::vec3(-0.1f, -2.2f, 0.0f));
		robotLegL = glm::scale(robotLegL, glm::vec3(0.2f, 2.0f, 0.2f));
		DrawCube(gCube, shaderProgramID, robotLegL, glm::vec3(0.8f, 0.5f, 0.5f));
		// 오른다리
		glm::mat4 robotLegR = robotBase;
		robotLegR = glm::rotate(robotLegR, glm::radians(angleLeg_X), glm::vec3(1.0f, 0.0f, 0.0f));
		robotLegR = glm::translate(robotLegR, glm::vec3(0.1f, -2.2f, 0.0f));
		robotLegR = glm::scale(robotLegR, glm::vec3(0.2f, 2.0f, 0.2f));
		DrawCube(gCube, shaderProgramID, robotLegR, glm::vec3(0.5f, 0.5f, 0.8f));

	}

	// 미니맵 그리기 
	glViewport(600, 600, 300, 300);
	glm::vec3 miniCamPos = glm::vec3(centerX - 2.0f, 100.0f, centerZ - 2.0f); // 높이
	glm::vec3 miniCamTarget = glm::vec3(centerX - 2.0f, 0.0f, centerZ - 2.0f);
	glm::vec3 miniCamUp = glm::vec3(0.0f, 0.0f, -1.0f); // z축 아래 방향

	glm::mat4 miniView = glm::lookAt(miniCamPos, miniCamTarget, miniCamUp);
	glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &miniView[0][0]);

	glm::mat4 miniProj = glm::ortho(
		-cubeCount_x / 2.0f, cubeCount_x / 2.0f,
		-cubeCount_z / 2.0f, cubeCount_z / 2.0f,
		1.0f, 100.0f
	);
	glUniformMatrix4fv(projLoc, 1, GL_FALSE, &miniProj[0][0]);

	// 바닥(미니맵용)
	glm::mat4 groundMini = glm::mat4(1.0f);
	groundMini = glm::translate(groundMini, glm::vec3(0.0f, 0.0f, 0.0f));
	groundMini = glm::scale(groundMini, glm::vec3(100.0f, 0.05f, 100.0f));
	DrawCube(gCube, shaderProgramID, groundMini, glm::vec3(0.8f, 0.8f, 0.8f));

	// 큐브들
	for (int i = 0; i < cubeCount_x; ++i)
	{
		for (int j = 0; j < cubeCount_z; j++)
		{
			if (!cubes[i][j].active) continue;  

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, glm::vec3(cubes[i][j].position.x,
				cubes[i][j].currentY, cubes[i][j].position.z + moveCubeZ));
			model = glm::scale(model, glm::vec3(1.0f, cubes[i][j].height, 1.0f));
			DrawCube(gCube, shaderProgramID, model, cubes[i][j].color);
		}
	}

	if (playerActive)
	{
		// 큐브 그리기
		// 공통
		glm::mat4 share = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
		share = glm::translate(share, glm::vec3(0.0f, 0.5f, -5.0f));
		share = glm::scale(share, glm::vec3(0.5f, 0.5f, 0.5f));

		// 로봇 그리기
		glm::mat4 robotBase = share;
		robotBase = glm::translate(robotBase, glm::vec3(moveX, 5.0f, moveZ));
		robotBase = glm::rotate(robotBase, glm::radians(angleY), glm::vec3(0.0f, 1.0f, 0.0f));
		// 머리
		glm::mat4 robotHead = robotBase;
		robotHead = glm::translate(robotHead, glm::vec3(0.0f, 0.0f, 0.0f));
		robotHead = glm::scale(robotHead, glm::vec3(1.0f, 1.0f, 1.0f));
		DrawCube(gCube, shaderProgramID, robotHead, glm::vec3(1.0f, 0.7f, 0.3f));

		// 코
		glm::mat4 robotNose = robotBase;
		robotNose = glm::translate(robotNose, glm::vec3(0.0f, 0.0f, 0.5f));
		robotNose = glm::scale(robotNose, glm::vec3(0.2f, 0.2f, 0.3f));
		DrawCube(gCube, shaderProgramID, robotNose, glm::vec3(1.0f, 0.0f, 0.0f));

		// 몸통
		glm::mat4 robotBody = robotBase;
		robotBody = glm::translate(robotBody, glm::vec3(0.0f, -1.0f, 0.0f));
		robotBody = glm::scale(robotBody, glm::vec3(1.0f, 1.5f, 1.0f));
		DrawCube(gCube, shaderProgramID, robotBody, glm::vec3(0.5f, 0.9f, 0.5f));

		// 왼팔
		glm::mat4 robotArmL = robotBase;
		robotArmL = glm::rotate(robotArmL, glm::radians(angleArm_X), glm::vec3(1.0f, 0.0f, 0.0f));
		robotArmL = glm::translate(robotArmL, glm::vec3(-0.5f, -1.0f, 0.0f));
		robotArmL = glm::scale(robotArmL, glm::vec3(0.3f, 1.2f, 0.3f));
		DrawCube(gCube, shaderProgramID, robotArmL, glm::vec3(0.7f, 0.6f, 0.7f));
		// 오른팔
		glm::mat4 robotArmR = robotBase;
		robotArmR = glm::rotate(robotArmR, glm::radians(-angleArm_X), glm::vec3(1.0f, 0.0f, 0.0f));
		robotArmR = glm::translate(robotArmR, glm::vec3(0.5f, -1.0f, 0.0f));
		robotArmR = glm::scale(robotArmR, glm::vec3(0.3f, 1.2f, 0.3f));
		DrawCube(gCube, shaderProgramID, robotArmR, glm::vec3(0.3f, 0.4f, 0.3f));

		// 왼다리
		glm::mat4 robotLegL = robotBase;
		robotLegL = glm::rotate(robotLegL, glm::radians(-angleLeg_X), glm::vec3(1.0f, 0.0f, 0.0f));
		robotLegL = glm::translate(robotLegL, glm::vec3(-0.1f, -2.2f, 0.0f));
		robotLegL = glm::scale(robotLegL, glm::vec3(0.2f, 2.0f, 0.2f));
		DrawCube(gCube, shaderProgramID, robotLegL, glm::vec3(0.8f, 0.5f, 0.5f));
		// 오른다리
		glm::mat4 robotLegR = robotBase;
		robotLegR = glm::rotate(robotLegR, glm::radians(angleLeg_X), glm::vec3(1.0f, 0.0f, 0.0f));
		robotLegR = glm::translate(robotLegR, glm::vec3(0.1f, -2.2f, 0.0f));
		robotLegR = glm::scale(robotLegR, glm::vec3(0.2f, 2.0f, 0.2f));
		DrawCube(gCube, shaderProgramID, robotLegR, glm::vec3(0.5f, 0.5f, 0.8f));

	}

	glutSwapBuffers();
}

GLvoid Reshape(int w, int h)
{
	width = w;
	height = h;
	glViewport(0, 0, w, h);
}