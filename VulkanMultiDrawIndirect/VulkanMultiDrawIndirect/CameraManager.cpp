#include "CameraManager.h"

using namespace DirectX;

CameraManager::CameraManager(float width, float height)
{
	_activeCamera = 0;

	float aspectRatio = width / height;
	float fov = 85.0f * XM_PI / 180.0f;
	Camera defaultCam;
	defaultCam.aspectRatio = aspectRatio;
	defaultCam.fov = fov;
	defaultCam.forward = XMFLOAT3(0.0f, 0.0f, 1.0f);
	defaultCam.up = XMFLOAT3(0.0f, 1.0f, 0.0f);
	defaultCam.nearPlane = 0.1f;
	defaultCam.farPlane = 2000.0f;
	defaultCam.position = XMFLOAT3(0.0f, 0.0f, -11.0f);
	_cameras.push_back(defaultCam);
	_activeCamera = 0;

}

CameraManager::~CameraManager()
{
}

unsigned CameraManager::AddCamera(float posX, float posY, float posZ, float dirX, float dirY, float dirZ, float fov, float aspectRatio, float upX, float upY, float upZ, float nearPlane, float farPlane)
{
	Camera cam;
	cam.position = XMFLOAT3(posX, posY, posZ);
	cam.forward = XMFLOAT3(dirX, dirY, dirZ);
	cam.up = XMFLOAT3(upX, upY, upZ);
	cam.fov = fov;
	cam.aspectRatio = aspectRatio;
	_cameras.push_back(cam);

	return _cameras.size() - 1;

}

unsigned CameraManager::CycleActiveCamera()
{

	_activeCamera = (_activeCamera + 1) % _cameras.size();
	return _activeCamera;
}

Camera CameraManager::GetActiveCamera() const
{
	return _cameras[_activeCamera];
}

void CameraManager::SetActiveCamera(unsigned id)
{
	if (id >= _cameras.size())
	{
		throw std::runtime_error("Tried to set nonexistant camera.");
		return;
	}
	_activeCamera = id;
}

//void CameraManager::FillPerFrameBuffer(PerFrameBuffer& pfb, int cameraID)
//{
//	if (cameraID == -1)
//		cameraID = _activeCamera;
//
//	XMVECTOR pos = XMLoadFloat3(&_cameras[cameraID].position);
//	XMVECTOR dir = XMLoadFloat3(&_cameras[cameraID].forward);
//	XMVECTOR up = XMLoadFloat3(&_cameras[cameraID].up);
//	XMMATRIX view = XMMatrixLookToLH(pos, dir, up);
//	XMMATRIX proj = XMMatrixPerspectiveFovLH(_cameras[cameraID].fov, _cameras[cameraID].aspectRatio, _cameras[cameraID].nearPlane, _cameras[cameraID].farPlane);
//
//	XMStoreFloat4x4(&pfb.View, XMMatrixTranspose(view));
//	XMStoreFloat4x4(&pfb.Proj, XMMatrixTranspose(proj));
//	XMStoreFloat4x4(&pfb.InvView, XMMatrixTranspose(XMMatrixInverse(nullptr, view)));
//	XMStoreFloat4x4(&pfb.ViewProj, XMMatrixTranspose(view * proj));
//	XMStoreFloat4x4(&pfb.InvViewProj, XMMatrixTranspose(XMMatrixInverse(nullptr, view * proj)));
//	XMStoreFloat4x4(&pfb.InvProj, XMMatrixTranspose(XMMatrixInverse(nullptr,proj)));
//	XMStoreFloat4(&pfb.CamPos, pos);
//}

void CameraManager::RotateActiveCamera(float degX, float degY, float degZ)
{
	float radX = degX * XM_PI / 180.0f;
	float radY = degY * XM_PI / 180.0f;
	float radZ = degZ * XM_PI / 180.0f;

	XMMATRIX rot = XMMatrixRotationRollPitchYaw(radX, radY, radZ);
	XMVECTOR dir = XMLoadFloat3(&_cameras[_activeCamera].forward);
	XMVECTOR up = XMLoadFloat3(&_cameras[_activeCamera].up);
	XMStoreFloat3(&_cameras[_activeCamera].forward, XMVector3Transform(dir,rot));
	XMStoreFloat3(&_cameras[_activeCamera].up, XMVector3Transform(up, rot));

}

void CameraManager::RotatePitch(float degrees)
{
	XMVECTOR horizontal = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
	XMVECTOR vertical = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMVECTOR up = XMLoadFloat3(&_cameras[_activeCamera].up);
	
	XMVECTOR forward = XMLoadFloat3(&_cameras[_activeCamera].forward);
	XMVECTOR r = XMVector3Cross(up, forward);

	float rad = -degrees * XM_PI / 180.0f;
	XMMATRIX rot = XMMatrixRotationAxis(r, rad);
	up = XMVector3Transform(up, rot);
	forward = XMVector3Transform(forward, rot);
	XMStoreFloat3(&_cameras[_activeCamera].up, up);
	XMStoreFloat3(&_cameras[_activeCamera].forward, forward);
}

void CameraManager::RotateYaw(float degrees)
{
	float rad = degrees * XM_PI / 180.0f;
	XMVECTOR up = XMLoadFloat3(&_cameras[_activeCamera].up);
	XMVECTOR forward = XMLoadFloat3(&_cameras[_activeCamera].forward);
	XMMATRIX rot = XMMatrixRotationY(rad);
	up = XMVector3Transform(up, rot);
	forward = XMVector3Transform(forward, rot);
	XMStoreFloat3(&_cameras[_activeCamera].up, up);
	XMStoreFloat3(&_cameras[_activeCamera].forward, forward);
	
}

void CameraManager::TranslateActiveCamera(float offsetX, float offsetY, float offsetZ)
{
	_cameras[_activeCamera].position.x += offsetX;
	_cameras[_activeCamera].position.y += offsetY;
	_cameras[_activeCamera].position.z += offsetZ;
}

void CameraManager::MoveForward(float amount)
{
	_cameras[_activeCamera].position.x += amount * _cameras[_activeCamera].forward.x;
	_cameras[_activeCamera].position.y += amount * _cameras[_activeCamera].forward.y;
	_cameras[_activeCamera].position.z += amount * _cameras[_activeCamera].forward.z;
}

void CameraManager::MoveRight(float amount)
{
	XMVECTOR up = XMLoadFloat3(&_cameras[_activeCamera].up);
	XMVECTOR forward = XMLoadFloat3(&_cameras[_activeCamera].forward);
	XMVECTOR r = XMVector3Cross(up, forward);
	_cameras[_activeCamera].position.x += amount * XMVectorGetX(r);
	_cameras[_activeCamera].position.y += amount * XMVectorGetY(r);
	_cameras[_activeCamera].position.z += amount * XMVectorGetZ(r);
}

void CameraManager::MoveUp(float amount)
{
	_cameras[_activeCamera].position.x += amount * _cameras[_activeCamera].up.x;
	_cameras[_activeCamera].position.y += amount * _cameras[_activeCamera].up.y;
	_cameras[_activeCamera].position.z += amount * _cameras[_activeCamera].up.z;
}

void CameraManager::SetCameraPosition(float posX, float posY, float posZ)
{
	_cameras[_activeCamera].position = XMFLOAT3(posX, posY, posZ);
}


float CameraManager::GetFarPlaneDistance() const
{
	return _cameras[_activeCamera].farPlane;
}

DirectX::XMMATRIX CameraManager::GetView() const
{
	return XMMatrixLookToLH(XMLoadFloat3(&_cameras[_activeCamera].position),
		XMLoadFloat3(&_cameras[_activeCamera].forward),
			XMLoadFloat3(&_cameras[_activeCamera].up));

}

DirectX::XMMATRIX CameraManager::GetProj() const
{
	return XMMatrixPerspectiveFovLH(_cameras[_activeCamera].fov,
		_cameras[_activeCamera].aspectRatio,
		_cameras[_activeCamera].nearPlane,
		_cameras[_activeCamera].farPlane);
}
