#ifndef _CAMERA_MANAGER_H_
#define _CAMERA_MANAGER_H_

#include <DirectXMath.h>
#include <vector>

struct Camera
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 forward;
	DirectX::XMFLOAT3 up;
	float fov;
	float aspectRatio;
	float nearPlane;
	float farPlane;
};


class CameraManager
{
public:
	CameraManager(float width, float height);
	~CameraManager();
	unsigned AddCamera(float posX, float posY, float posZ,
		float dirX, float dirY, float dirZ,
		float fov, float aspectRatio,
		float upX = 0.0f, float upY = 1.0f, float upZ = 0.0f,
		float nearPlane = 1.0f, float farPlane = 100.0f);
	unsigned CycleActiveCamera();
	Camera GetActiveCamera()const;
	void SetActiveCamera(unsigned id);
//	void FillPerFrameBuffer(PerFrameBuffer& pfb, int cameraID = -1);
	void RotateActiveCamera(float degX, float degY, float degZ);
	void RotatePitch(float degrees);
	void RotateYaw(float degrees);
	void TranslateActiveCamera(float offsetX, float offsetY, float offsetZ);
	void MoveForward(float amount);
	void MoveRight(float amount);
	void MoveUp(float amount);
	void SetCameraPosition(float posX, float posY, float posZ);
	
	float GetFarPlaneDistance() const;
	DirectX::XMMATRIX GetView() const;
	DirectX::XMMATRIX GetProj() const;
private:
	std::vector<Camera> _cameras;
	int _activeCamera;

};

#endif
