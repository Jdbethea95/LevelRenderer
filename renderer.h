#include <d3dcompiler.h>	// required for compiling shaders on the fly, consider pre-compiling instead
#include "FSLogo.h"
#include "h2bParser.h"
#include "load_data_oriented.h"
#include "Helper.h"
#pragma comment(lib, "d3dcompiler.lib") 

//converions notes
/*Lines updated
* 161-165 (declaration of level data)
* 201 (vertex buffer)
* 221 (index buffer)
* 381 (drawindex)
* 428 (stride)
* initalizedpipeline, compileGSShader
*/

enum Cameras
{
	MainCamera, ArialCamera, LeftCamera, RightCamera
};

void PrintLabeledDebugString(const char* label, const char* toPrint)
{
	std::cout << label << toPrint << std::endl;
#if defined WIN32 //OutputDebugStringA is a windows-only function 
	OutputDebugStringA(label);
	OutputDebugStringA(toPrint);
#endif
}

/// <summary>
/// creates a GVECTORF
/// </summary>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="z"></param>
/// <param name="w">: Set to 1 by default</param>
/// <returns></returns>
GW::MATH::GVECTORF Vector4(float x, float y, float z, float w = 1)
{
	GW::MATH::GVECTORF vector = GW::MATH::GVECTORF();
	vector.x = x; vector.y = y; vector.z = z; vector.w = w;
	return vector;
}

/// <summary>
/// creates a GVECTORF by calling Vector4 and calls TranslateLocalF
/// </summary>
/// <param name="_matrix"></param>
/// <param name="x"></param>
/// <param name="y"></param>
/// <param name="z"></param>
void Translation(GW::MATH::GMATRIXF& _matrix, float x, float y, float z)
{
	GW::GReturn ret = GW::GReturn();
	ret = GW::MATH::GMatrix::TranslateLocalF(_matrix, Vector4(x, y, z), _matrix);
}

struct SceneData
{
	GW::MATH::GVECTORF vec_SunDirection, vec_SunColor;
	GW::MATH::GMATRIXF mat_View, mat_Projection;
	GW::MATH::GVECTORF sunAmbient, cameraPos;
};

struct MeshData
{
	GW::MATH::GMATRIXF mat_World[200]; //turn to array[200]
	H2B::ATTRIBUTES material[200];	//turn to array[200]
};

struct GSData
{
	GW::MATH::GVECTORF index;
};

struct IdData
{
	GW::MATH::GVECTORF tmIDS; //these are the id's for the transform array and material array
};								//for use in the vertex and pixel shaders


// TODO: Part 2B 
// TODO: Part 4E 

// Creation, Rendering & Cleanup
class Renderer
{
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GDirectX11Surface d3d;

	// what we need at a minimum to draw a triangle
	Microsoft::WRL::ComPtr<ID3D11Buffer>		vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	pixelShader;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>	vertexFormat;
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> gsShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer>		indexBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> sceneConstantBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> meshConstantBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> gsConstantBuffer = nullptr;
	Microsoft::WRL::ComPtr<ID3D11Buffer> idConstantBuffer = nullptr; //another buffer for id's is needed. transform and count
	// TODO: Part 2A
	//math libraries
	GW::MATH::GMatrix matrixMath;
	GW::MATH::GVector vectorMath;

	//Space Matrices for GPU/Vertex Shader
	GW::MATH::GMATRIXF world = GW::MATH::GMATRIXF();
	GW::MATH::GMATRIXF wordWorld = GW::MATH::GMATRIXF();
	GW::MATH::GMATRIXF view = GW::MATH::GMATRIXF();
	GW::MATH::GMATRIXF projection = GW::MATH::GMATRIXF();
	GW::MATH::GVECTORF lightDirection = GW::MATH::GVECTORF();
	GW::MATH::GVECTORF lightColor = GW::MATH::GVECTORF();
	OBJ_ATTRIBUTES material; //probably wont be used. may delete later

	GW::MATH::GMATRIXF cameras[4] = { GW::MATH::GMATRIXF() };
	Cameras currentCam = Cameras::MainCamera;

	SceneData sceneData;
	MeshData meshData;
	GSData gsData;
	IdData idTMData;

	D3D11_VIEWPORT viewports[2];

	GW::INPUT::GInput gateInput;
	GW::INPUT::GController gateController;
	//Clock data for deltaTime calculation in rotation of the world matrix
	std::chrono::steady_clock clock;
	std::chrono::time_point<std::chrono::steady_clock> ping;
	std::chrono::time_point<std::chrono::steady_clock> lastPing;

	// TODO: Part 2B 
	Level_Data dataOrientedLoader;
	unsigned currVIndex;

public:
	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GDirectX11Surface _d3d)
	{
		win = _win;
		d3d = _d3d;


		//new viewport
		unsigned int screen_width, screen_height;
		win.GetWidth(screen_width);
		win.GetHeight(screen_height);

		viewports[1].Width = screen_width / 4;
		viewports[1].Height = screen_height / 4;
		viewports[1].MinDepth = 0;
		viewports[1].MaxDepth = 1;
		viewports[1].TopLeftX = 0;
		viewports[1].TopLeftY = 0;



		viewports[0].Width = screen_width;
		viewports[0].Height = screen_height;
		viewports[0].MinDepth = 0;
		viewports[0].MaxDepth = 1;
		viewports[0].TopLeftX = 0;
		viewports[0].TopLeftY = 0;

		currVIndex = 0;
		gsData.index.x = currVIndex; //there is a reason, trust me. May not be a good reason, but there is one.

		// TODO: Part 2A 
		GW::GReturn ret = GW::GReturn();
		ret = matrixMath.Create(); //FAILURE_UNSUPPORTED (-8)
		vectorMath.Create();


		//world matrix creation for mesh data ####CLEANUP####
		ret = matrixMath.IdentityF(world); //needs to rotate on the y axis. 
		world = GW::MATH::GIdentityMatrixF; //GW::MATH::GIdentityMatrixF this works.
		ret = matrixMath.IdentityF(wordWorld);
		//meshData.mat_World = wordWorld;

		//view matrix and camera creation for scene data

		//Main Camera
		ret = matrixMath.IdentityF(cameras[0]);
		Translation(cameras[0], 0.75f, 0.45f, -1.0f); //creates a GVECTORF and calls TranslateLocalF
		matrixMath.LookAtLHF(cameras[0].row4, Vector4(0.15f, 0.45f, 0), Vector4(0, 1, 0), cameras[0]);
		matrixMath.InverseF(cameras[0], cameras[0]);
		sceneData.cameraPos = cameras[0].row4;
		matrixMath.InverseF(cameras[0], cameras[0]);
		sceneData.mat_View = cameras[0];


		//projection matrix creation scene data
		float ratio;
		d3d.GetAspectRatio(ratio);
		ret = matrixMath.ProjectionDirectXLHF(65.0f, ratio, 0.1f, 100.0f, projection); //FAILURE (-1)
		sceneData.mat_Projection = projection;

		//light direction creation for scene data
		lightDirection = Vector4(-1, -1, 2); //creates a GVECTORF
		vectorMath.NormalizeF(lightDirection, lightDirection);
		sceneData.vec_SunDirection = lightDirection;

		//light color creation for scene data
		lightColor = Vector4(0.9f, 0.9f, 1.0f); // w/a is set to 1 automatically in Vector4
		sceneData.vec_SunColor = lightColor;

		//assigning Ambient light
		sceneData.sunAmbient = Vector4(0.25f, 0.25f, 0.35f);
		//assigning material
		//meshData.material = FSLogo_materials[0].attrib; ####CLEANUP####


		GW::SYSTEM::GLog log; // handy for logging any messages/warning/errors begin loading level
		log.Create("../LevelLoaderLog.txt"); //kept for testing purposes
		log.EnableConsoleLogging(true); // mirror output to the console
		log.Log("Start Program.");

		dataOrientedLoader.LoadLevel("../MyGameLevel.txt", "../Models", log);
		

		for (int i = 0; i < dataOrientedLoader.levelTransforms.size(); i++)
		{
			meshData.mat_World[i] = dataOrientedLoader.levelTransforms[i]; //assigning to world array
		}

		for (int j = 0; j < dataOrientedLoader.levelMaterials.size(); j++)
		{
			meshData.material[j] = dataOrientedLoader.levelMaterials[j].attrib; //assigning to material array
		}


		//from blender cameras
		//dataOrientedLoader.levelTransforms, camera matrices are not loaded in data.

		SetLevelCameras();

		gateInput.Create(win);
		gateController.Create();

		// TODO: Part 2B assigned the variables above to the struct members
		// TODO: Part 4E 
		IntializeGraphics();
	}

	void ONRESIZE(GW::SYSTEM::GWindow why, bool& shouldI)
	{
		if (!shouldI)
			return;
		
		unsigned int screen_width, screen_height;
		why.GetWidth(screen_width);
		why.GetHeight(screen_height);

		viewports[1].Width = screen_width / 4;
		viewports[1].Height = screen_height / 4;

		viewports[0].Width = screen_width;
		viewports[0].Height = screen_height;		
		shouldI = false;
	}

private:
	//constructor helper functions
	void IntializeGraphics()
	{
		ID3D11Device* creator;
		d3d.GetDevice((void**)&creator);

		InitializeVertexBuffer(creator);
		// TODO: Part 1G: creating a indexbuffer that accepts the unsigned int array from image header.
		InitializeIndexBuffer(creator);
		// TODO: Part 2C 
		CreateGPUShaderBuffer(creator, &sceneData, sizeof(SceneData), sceneConstantBuffer.GetAddressOf());
		CreateGPUShaderBuffer(creator, &meshData, sizeof(MeshData), meshConstantBuffer.GetAddressOf());
		CreateGPUShaderBuffer(creator, &meshData, sizeof(GSData), gsConstantBuffer.GetAddressOf());
		CreateGPUShaderBuffer(creator, &idTMData, sizeof(IdData), idConstantBuffer.GetAddressOf());

		InitializePipeline(creator);

		// free temporary handle
		creator->Release();
	}

	void InitializeVertexBuffer(ID3D11Device* creator)
	{

		// TODO: Part 1C replaced vert with the FSlogo Vertex Array: DO NOT FORGET TO UPDATE THE STRIDE AND AMOUNT TO DRAW
		CreateVertexBuffer(creator, dataOrientedLoader.levelVertices.data(), sizeof(H2B::VERTEX) * dataOrientedLoader.levelVertices.size()); // reminder: fix hard coded value
	}

	void CreateGPUShaderBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes, ID3D11Buffer** buffer)
	{
		D3D11_SUBRESOURCE_DATA subersourceData{ data, 0, 0 };
		CD3D11_BUFFER_DESC bufferDesc{ sizeInBytes, D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE };
		creator->CreateBuffer(&bufferDesc, &subersourceData, buffer);
	}

	void CreateVertexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_VERTEX_BUFFER);
		creator->CreateBuffer(&bDesc, &bData, vertexBuffer.GetAddressOf());
	}

	void InitializeIndexBuffer(ID3D11Device* creator)
	{
		//A box WITHIN A BOX!!! initialized function made for uniformity.
		CreateIndexBuffer(creator, dataOrientedLoader.levelIndices.data(), sizeof(unsigned) * dataOrientedLoader.levelIndices.size());
	}

	void CreateIndexBuffer(ID3D11Device* creator, const void* data, unsigned int sizeInBytes)
	{
		D3D11_SUBRESOURCE_DATA bData = { data, 0, 0 };
		CD3D11_BUFFER_DESC bDesc(sizeInBytes, D3D11_BIND_INDEX_BUFFER);
		creator->CreateBuffer(&bDesc, &bData, indexBuffer.GetAddressOf());
	}

	void InitializePipeline(ID3D11Device* creator)
	{
		UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#if _DEBUG
		compilerFlags |= D3DCOMPILE_DEBUG;
#endif
		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob = CompileVertexShader(creator, compilerFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> gsBlob = CompileGSShader(creator, compilerFlags);
		Microsoft::WRL::ComPtr<ID3DBlob> psBlob = CompilePixelShader(creator, compilerFlags); 
		CreateVertexInputLayout(creator, vsBlob);
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompileVertexShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string vertexShaderSource = ReadFileIntoString("../Shaders/VertexShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> vsBlob, errors;

		HRESULT compilationResult =
			D3DCompile(vertexShaderSource.c_str(), vertexShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "vs_4_0", compilerFlags, 0,
				vsBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreateVertexShader(vsBlob->GetBufferPointer(),
				vsBlob->GetBufferSize(), nullptr, vertexShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Vertex Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return vsBlob;
	}

	Microsoft::WRL::ComPtr<ID3DBlob> CompilePixelShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string pixelShaderSource = ReadFileIntoString("../Shaders/PixelShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> psBlob, errors;

		HRESULT compilationResult =
			D3DCompile(pixelShaderSource.c_str(), pixelShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "ps_4_0", compilerFlags, 0,
				psBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreatePixelShader(psBlob->GetBufferPointer(),
				psBlob->GetBufferSize(), nullptr, pixelShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Pixel Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return psBlob;
	}
	Microsoft::WRL::ComPtr<ID3DBlob> CompileGSShader(ID3D11Device* creator, UINT compilerFlags)
	{
		std::string GSShaderSource = ReadFileIntoString("GeometryShader.hlsl");

		Microsoft::WRL::ComPtr<ID3DBlob> gsBlob, errors;

		HRESULT compilationResult =
			D3DCompile(GSShaderSource.c_str(), GSShaderSource.length(),
				nullptr, nullptr, nullptr, "main", "gs_4_0", compilerFlags, 0,
				gsBlob.GetAddressOf(), errors.GetAddressOf());

		if (SUCCEEDED(compilationResult))
		{
			creator->CreateGeometryShader(gsBlob->GetBufferPointer(),
				gsBlob->GetBufferSize(), nullptr, gsShader.GetAddressOf());
		}
		else
		{
			PrintLabeledDebugString("Geometry Shader Errors:\n", (char*)errors->GetBufferPointer());
			abort();
			return nullptr;
		}

		return gsBlob;
	}

	void CreateVertexInputLayout(ID3D11Device* creator, Microsoft::WRL::ComPtr<ID3DBlob>& vsBlob)
	{
		// TODO: Part 1E changed DXGI_FORMAT_R32G32_FLOAT to DXGI_FORMAT_R32G32B32_FLOAT

		D3D11_INPUT_ELEMENT_DESC attributes[3];

		attributes[0].SemanticName = "POSITION";
		attributes[0].SemanticIndex = 0;
		attributes[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[0].InputSlot = 0;
		attributes[0].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[0].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[0].InstanceDataStepRate = 0;

		attributes[1].SemanticName = "UVW";
		attributes[1].SemanticIndex = 0;
		attributes[1].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[1].InputSlot = 0;
		attributes[1].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[1].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[1].InstanceDataStepRate = 0;

		attributes[2].SemanticName = "NORMAL";
		attributes[2].SemanticIndex = 0;
		attributes[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		attributes[2].InputSlot = 0;
		attributes[2].AlignedByteOffset = D3D11_APPEND_ALIGNED_ELEMENT;
		attributes[2].InputSlotClass = D3D11_INPUT_PER_VERTEX_DATA;
		attributes[2].InstanceDataStepRate = 0;

		creator->CreateInputLayout(attributes, ARRAYSIZE(attributes),
			vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(),
			vertexFormat.GetAddressOf());
	}

	void SetLevelCameras() 
	{
		matrixMath.InverseF(dataOrientedLoader.cameraTransforms[0], cameras[1]);
		matrixMath.InverseF(dataOrientedLoader.cameraTransforms[1], cameras[2]);
		matrixMath.InverseF(dataOrientedLoader.cameraTransforms[2], cameras[3]);
	}


public:
	void Render()
	{
		PipelineHandles curHandles = GetCurrentPipelineHandles();
		SetUpPipeline(curHandles);

		D3D11_MAPPED_SUBRESOURCE mappedSubresource;

		SwitchCameras();
		SwitchLevels();

		//SCENE DATA ASSIGMENT
		curHandles.context->Map(sceneConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);

		switch (currentCam) // the switch is present to keep cameras and cam movement seperate
		{
		case MainCamera:
			sceneData.mat_View = cameras[0];
			break;
		case ArialCamera:
			sceneData.mat_View = cameras[1];
			break;
		case LeftCamera:
			sceneData.mat_View = cameras[2];
			break;
		case RightCamera:
			sceneData.mat_View = cameras[3];
			break;
		}

		memcpy_s(mappedSubresource.pData, sizeof(sceneData), &sceneData, sizeof(sceneData));
		curHandles.context->Unmap(sceneConstantBuffer.Get(), 0);


		// TODO: Part 1H set the index buffer to the input assembler.
		curHandles.context->IASetIndexBuffer(indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

		ID3D11Buffer* shaderConstantBuffers[] = { sceneConstantBuffer.Get(), meshConstantBuffer.Get(), idConstantBuffer.Get() };
		ID3D11Buffer* gsConstant[] = { gsConstantBuffer.Get() };
		curHandles.context->VSSetConstantBuffers(0, 3, shaderConstantBuffers);
		curHandles.context->GSSetConstantBuffers(0, 1, gsConstant);
		curHandles.context->PSSetConstantBuffers(0, 3, shaderConstantBuffers);

		//loop viewport index for GS Shader. this can work without this outer loop but it looks faded.
		for (int v = 0; v < 2; v++)
		{

			//GS DATA ASSIGNMENT
			curHandles.context->Map(gsConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
			gsData.index.x = v;
			memcpy_s(mappedSubresource.pData, sizeof(GSData), &gsData, sizeof(gsData));
			curHandles.context->Unmap(gsConstantBuffer.Get(), 0);

			//Loop not looping right
			for (int i = 0; i < dataOrientedLoader.levelModels.size(); i++)
			{


				//Setting constant buffers

				// TODO: Part 3B 
				// TODO: Part 3C 
				// TODO: Part 4D 

				int batchStart = dataOrientedLoader.levelModels[i].batchStart;
				int indexStart = dataOrientedLoader.levelModels[i].indexStart;
				int vertexStart = dataOrientedLoader.levelModels[i].vertexStart;


				HRESULT hCode = curHandles.context->Map(idConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);


				idTMData.tmIDS = Vector4(dataOrientedLoader.levelInstances[i].transformStart,
					dataOrientedLoader.levelModels[i].materialStart, -1, -1);
				memcpy_s(mappedSubresource.pData, sizeof(IdData), &idTMData, sizeof(idTMData));
				curHandles.context->Unmap(idConstantBuffer.Get(), 0);

				for (int j = 0; j < dataOrientedLoader.levelModels[i].meshCount; j++)
				{

					int index = batchStart + j;

					HRESULT hCode = curHandles.context->Map(idConstantBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource);
					idTMData.tmIDS.y = dataOrientedLoader.levelMeshes[index].materialIndex + dataOrientedLoader.levelModels[i].materialStart;
					memcpy_s(mappedSubresource.pData, sizeof(IdData), &idTMData, sizeof(idTMData));
					curHandles.context->Unmap(idConstantBuffer.Get(), 0);

					//somewhere a vector subscript is out of range.
					//D3D11_VIEWPORT temp[1] = { viewports[1] };
					//curHandles.context->RSSetViewports(1, temp);
					;

					curHandles.context->DrawIndexedInstanced(dataOrientedLoader.levelMeshes[index].drawInfo.indexCount,
						dataOrientedLoader.levelInstances[i].transformCount,
						dataOrientedLoader.levelMeshes[index].drawInfo.indexOffset + indexStart,
						vertexStart, 0); //can this be 0

					//D3D11_VIEWPORT temp2[1] = { viewports[0] };
					//curHandles.context->RSSetViewports(1, temp2);
					//
					//curHandles.context->DrawIndexedInstanced(dataOrientedLoader.levelMeshes[index].drawInfo.indexCount,
					//	dataOrientedLoader.levelInstances[i].transformCount,
					//	dataOrientedLoader.levelMeshes[index].drawInfo.indexOffset + indexStart,
					//	vertexStart, 0); //can this be 0

				}




				//8532, 0, 0 does produce the whole meshe		
				// TODO: Part 1D  
			}



		}

		ReleasePipelineHandles(curHandles);
	}

	void UpdateCamera()
	{

		if (currentCam != Cameras::MainCamera)
			return;

		GW::MATH::GMATRIXF cameraMatrix;
		matrixMath.InverseF(cameras[0], cameraMatrix);

		float yChange = 0, zChange = 0, xChange = 0, pitchChange = 0, yawChange = 0,//changeStates
			spaceKey = 0, leftShiftKey = 0, wKey = 0, aKey = 0, sKey = 0, dKey = 0, mouseX = 0, mouseY = 0,//PC
			padRightTrigger = 0, padLeftTrigger = 0, padStickLX = 0, padStickLY = 0, padStickRY = 0, padStickRX = 0, sprintKey = 0;//gamepad


		float cameraSpeed = 0;

		std::chrono::milliseconds ms{ 60 };
		ping = clock.now();
		float deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(ping - lastPing).count() / 10000.0f;
		lastPing = ping;

		//G_KEY_RIGHTSHIFT
		gateInput.GetState(G_KEY_LEFTCONTROL, sprintKey);
		if (sprintKey == 0)
			cameraSpeed = 10.0f;
		else cameraSpeed = 40.0f;


		gateInput.GetState(G_KEY_SPACE, spaceKey); gateInput.GetState(G_KEY_LEFTSHIFT, leftShiftKey);
		gateController.GetState(7, G_RIGHT_TRIGGER_AXIS, padRightTrigger); gateController.GetState(7, G_LEFT_TRIGGER_AXIS, padLeftTrigger);
		yChange = (spaceKey - leftShiftKey) + (padRightTrigger - padLeftTrigger);
		cameraMatrix.row4.y += yChange * cameraSpeed * deltaTime;


		float perframeSpeed = cameraSpeed * deltaTime;
		gateInput.GetState(G_KEY_W, wKey); gateInput.GetState(G_KEY_A, aKey); gateController.GetState(7, G_LY_AXIS, padStickLX);
		gateInput.GetState(G_KEY_S, sKey); gateInput.GetState(G_KEY_D, dKey); gateController.GetState(7, G_LX_AXIS, padStickLY);

		zChange = (wKey - sKey) + padStickLY;
		xChange = (dKey - aKey) + padStickLX;

		if (aKey != 0)
			int f = 5;

		Translation(cameraMatrix, xChange * perframeSpeed, 0, zChange * perframeSpeed);


		unsigned int screenHeight = 0, screenWidth = 0;
		win.GetHeight(screenHeight); win.GetWidth(screenWidth);


		float rotateSpeed = 3.141592653589793238f * deltaTime;
		gateInput.GetMouseDelta(mouseX, mouseY);
		gateController.GetState(7, G_RY_AXIS, padStickRY);

		if (mouseX == 1 || mouseX == -1)
			mouseX = 0;

		if (mouseY == 1 || mouseY == -1)
			mouseY = 0;

		pitchChange = (65.0f * mouseY / screenHeight) + padStickRY * rotateSpeed * -1;


		matrixMath.RotateXLocalF(cameraMatrix, Degrees2Radians(pitchChange), cameraMatrix);

		float ratio;
		d3d.GetAspectRatio(ratio);
		gateController.GetState(7, G_RX_AXIS, padStickRX);
		yawChange = 65.0f * ratio * mouseX / screenWidth + padStickRX * rotateSpeed * -1;
		matrixMath.RotateYGlobalF(cameraMatrix, Degrees2Radians(yawChange), cameraMatrix);

		matrixMath.InverseF(cameraMatrix, cameras[0]);

	}

private:
	struct PipelineHandles
	{
		ID3D11DeviceContext* context;
		ID3D11RenderTargetView* targetView;
		ID3D11DepthStencilView* depthStencil;
	};

	PipelineHandles GetCurrentPipelineHandles()
	{
		PipelineHandles retval;
		d3d.GetImmediateContext((void**)&retval.context);
		d3d.GetRenderTargetView((void**)&retval.targetView);
		d3d.GetDepthStencilView((void**)&retval.depthStencil);
		return retval;
	}

	void SetUpPipeline(PipelineHandles handles)
	{
		SetRenderTargets(handles);
		SetVertexBuffers(handles);
		SetShaders(handles);

		handles.context->IASetInputLayout(vertexFormat.Get());
		handles.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		handles.context->RSSetViewports(2, viewports);
	}

	void SetRenderTargets(PipelineHandles handles)
	{
		ID3D11RenderTargetView* const views[] = { handles.targetView };
		handles.context->OMSetRenderTargets(ARRAYSIZE(views), views, handles.depthStencil);
	}

	void SetVertexBuffers(PipelineHandles handles)
	{
		const UINT strides[] = { sizeof(H2B::VERTEX) }; // TODO: Part 1E changed to size of OBJ_VEC3, may need to be _OBJ_VERT_
		const UINT offsets[] = { 0 };								//it was indeed _OBJ_VERT_ lol
		ID3D11Buffer* const buffs[] = { vertexBuffer.Get() };
		handles.context->IASetVertexBuffers(0, ARRAYSIZE(buffs), buffs, strides, offsets);
	}

	void SetShaders(PipelineHandles handles)
	{
		handles.context->VSSetShader(vertexShader.Get(), nullptr, 0);
		handles.context->PSSetShader(pixelShader.Get(), nullptr, 0);
		handles.context->GSSetShader(gsShader.Get(), nullptr, 0);
	}

	void ReleasePipelineHandles(PipelineHandles toRelease)
	{
		toRelease.depthStencil->Release();
		toRelease.targetView->Release();
		toRelease.context->Release();
	}

	void SwitchCameras()
	{

		float camera1 = 0, camera2 = 0, camera3 = 0, camera4 = 0;
		gateInput.GetState(G_KEY_1, camera1); gateInput.GetState(G_KEY_2, camera2); gateInput.GetState(G_KEY_3, camera3); gateInput.GetState(G_KEY_4, camera4);

		if (camera1 != 0)
		{
			currentCam = Cameras::ArialCamera;
		}
		else if (camera2 != 0) 
		{
			currentCam = Cameras::LeftCamera;
		}
		else if (camera3 != 0)
		{
			currentCam = Cameras::RightCamera;
		}
		else if (camera4 != 0)
		{
			currentCam = Cameras::MainCamera;
		}

	}

	void SwitchLevels() 
	{

		float level2 = 0, level1 = 0;
		gateInput.GetState(G_KEY_6, level2); gateInput.GetState(G_KEY_7, level1);
		

		if (level2 != 0)
		{

			GW::SYSTEM::GLog log; // handy for logging any messages/warning/errors begin loading level
			log.Create("../LevelLoaderLog.txt"); //kept for testing purposes
			log.EnableConsoleLogging(true); // mirror output to the console
			log.Log("Start Program.");

			dataOrientedLoader.LoadLevel("../OtherLevel.txt", "../Models", log);

			for (int i = 0; i < dataOrientedLoader.levelTransforms.size(); i++)
			{
				meshData.mat_World[i] = dataOrientedLoader.levelTransforms[i]; //assigning to world array
			}

			for (int j = 0; j < dataOrientedLoader.levelMaterials.size(); j++)
			{
				meshData.material[j] = dataOrientedLoader.levelMaterials[j].attrib; //assigning to material array
			}


			//Translation(cameras[0], 0.75f, 0.45f, -1.0f); //creates a GVECTORF and calls TranslateLocalF
			cameras[0].row4 = Vector4(0.75f, 3.45f, -1.0f);
			matrixMath.LookAtLHF(cameras[0].row4, Vector4(0.15f, 0.45f, 0), Vector4(0, 1, 0), cameras[0]);
			matrixMath.InverseF(cameras[0], cameras[0]);
			sceneData.cameraPos = cameras[0].row4;
			matrixMath.InverseF(cameras[0], cameras[0]);
			sceneData.mat_View = cameras[0];


			SetLevelCameras();
			IntializeGraphics();
		}
		else if (level1 != 0) 
		{
			dataOrientedLoader.UnloadLevel();
			GW::SYSTEM::GLog log; // handy for logging any messages/warning/errors begin loading level
			log.Create("../LevelLoaderLog.txt"); //kept for testing purposes
			log.EnableConsoleLogging(true); // mirror output to the console
			log.Log("Start Program.");

			dataOrientedLoader.LoadLevel("../MyGameLevel.txt", "../Models", log);


			for (int i = 0; i < dataOrientedLoader.levelTransforms.size(); i++)
			{
				meshData.mat_World[i] = dataOrientedLoader.levelTransforms[i]; //assigning to world array
			}

			for (int j = 0; j < dataOrientedLoader.levelMaterials.size(); j++)
			{
				meshData.material[j] = dataOrientedLoader.levelMaterials[j].attrib; //assigning to material array
			}

			IntializeGraphics();
		}
	}

public:
	~Renderer()
	{
		// ComPtr will auto release so nothing to do here yet 
	}
};
