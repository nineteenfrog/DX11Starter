#include "Camera.h"
#include "Input.h"

using namespace DirectX;

Camera::Camera(
	float x, float y, float z, 
	float moveSpeed, 
	float mouseLookSpeed,
	float fov, 
	float aspectRatio)
	:
	moveSpeed(moveSpeed),
	mouseLookSpeed(mouseLookSpeed),
	fov(fov),
	aspectRatio(aspectRatio)
{
	nearP = 0.1f;
	farP = 1000;
	//setposition
	transform = Transform();
	transform.SetPosition(x, y, z);
	//update matrices
	UpdateViewMatrix();
	UpdateProjectionMatrix(aspectRatio);
}

Camera::~Camera()
{
}

void Camera::Update(float dt)
{

	//update constant buffer 
	//add new view and projection matrix in shader and buffer sturct


	//getkeyboard input
	
	Input& input = Input::GetInstance();

	if (input.KeyDown('W')) { transform.MoveRelative(0, 0, moveSpeed * dt); }
	if (input.KeyDown('A')) { transform.MoveRelative(-moveSpeed * dt, 0, 0); }
	if (input.KeyDown('S')) { transform.MoveRelative(0, 0, -moveSpeed * dt); }
	if (input.KeyDown('D')) { transform.MoveRelative(moveSpeed * dt, 0, 0); }
	if (input.KeyDown('Q')) { transform.MoveRelative(0, moveSpeed * dt, 0); }
	if (input.KeyDown('E')) { transform.MoveRelative(0, -moveSpeed * dt, 0); }

	if (input.MouseLeftDown()) {
		int xDiff = mouseLookSpeed * input.GetMouseXDelta();
		int yDiff = mouseLookSpeed * input.GetMouseYDelta();

		transform.Rotate(yDiff * dt, xDiff * dt, 0); //not sure why the movement is flipped
	}

	UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
	XMVECTOR position = { transform.GetPosition().x,transform.GetPosition().y ,transform.GetPosition().z };
	XMVECTOR direction = { transform.GetForward().x, transform.GetForward().y ,transform.GetForward().z };
	XMMATRIX view = XMMatrixLookToLH(
		position,
		direction,
		XMVECTOR({ 0,1,0 }));

	XMStoreFloat4x4(&viewMatrix, view);
}

void Camera::UpdateProjectionMatrix(float aspectRatio)
{

	XMMATRIX proj = XMMatrixPerspectiveFovLH(
		fov,
		aspectRatio,
		0.01f,  //near clip dist
		1000.0f); //far clip dist

	XMStoreFloat4x4(&projectionMatrix, proj);
}

Transform* Camera::GetTransform()
{
	return &transform;
}

DirectX::XMFLOAT4X4 Camera::GetView()
{
	return viewMatrix;
}

DirectX::XMFLOAT4X4 Camera::GetProjection()
{
	return projectionMatrix;
}

float Camera::GetFov()
{
	return fov;
}
