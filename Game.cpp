#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "BufferStructs.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true)				// Show extra stats (fps) in title bar?
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	//  - You'll be expanding and/or replacing these later
	LoadShaders();
	CreateGeometry();


	// Set initial graphics API state
	//  - These settings persist until we change them
	//  - Some of these, like the primitive topology & input layout, probably won't change
	//  - Others, like setting shaders, will need to be moved elsewhere later
	{
		// Tell the input assembler (IA) stage of the pipeline what kind of
		// geometric primitives (points, lines or triangles) we want to draw.  
		// Essentially: "What kind of shape should the GPU draw with our vertices?"
		context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}

	//Initialize ImGui stuff
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGui_ImplWin32_Init(hWnd);
		ImGui_ImplDX11_Init(device.Get(), context.Get());

		ImGui::StyleColorsDark();
	}
	// Get size as the next multiple of 16 (instead of hardcoding a size here!)
	unsigned int size = sizeof(VertexShaderExternalData);
	size = (size + 15) / 16 * 16; // This will work even if the struct size changes

	// Describe the constant buffer
	D3D11_BUFFER_DESC cbDesc = {}; // Sets struct to all zeros
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = size; // Must be a multiple of 16
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;

	device->CreateBuffer(&cbDesc, 0, vsConstantBuffer.GetAddressOf());

	camera[0] = std::make_shared<Camera>(10.0f, 0.0f, -10.0f, 5.0f, 10.0f, XM_PI / 2, (float)this->windowWidth / this->windowHeight);
	camera[1] = std::make_shared<Camera>(0.0f, 0.0f, -10.0f, 5.0f, 10.0f, XM_PI / 3, (float)this->windowWidth / this->windowHeight);
	camera[2] = std::make_shared<Camera>(-10.0f, 0.0f, -10.0f, 5.0f, 10.0f, XM_PI / 4, (float)this->windowWidth / this->windowHeight);

}

// --------------------------------------------------------
// Loads shaders from compiled shader object (.cso) files
// and also created the Input Layout that describes our 
// vertex data to the rendering pipeline. 
// - Input Layout creation is done here because it must 
//    be verified against vertex shader byte code
// - We'll have that byte code already loaded below
// --------------------------------------------------------
void Game::LoadShaders()
{
	//vertex shader
	vertexShader = std::make_shared<SimpleVertexShader>(
		device,
		context,
		FixPath(L"VertexShader.cso").c_str());
	//pixel shader
	vertexShader = std::make_shared<SimpleVertexShader>(
		device,
		context,
		FixPath(L"PixelShader.cso").c_str());
}



// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateGeometry()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green = XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue = XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);
	XMFLOAT4 magenta = XMFLOAT4(0.5f, 0.0f, 0.5f, 1.0f);

	// Set up the vertices of the triangle we would like to draw
	// - We're going to copy this array, exactly as it exists in CPU memory
	//    over to a Direct3D-controlled data structure on the GPU (the vertex buffer)
	// - Note: Since we don't have a camera or really any concept of
	//    a "3d world" yet, we're simply describing positions within the
	//    bounds of how the rasterizer sees our screen: [-1 to +1] on X and Y
	// - This means (0,0) is at the very center of the screen.
	// - These are known as "Normalized Device Coordinates" or "Homogeneous 
	//    Screen Coords", which are ways to describe a position without
	//    knowing the exact size (in pixels) of the image/window/etc.  
	// - Long story short: Resizing the window also resizes the triangle,
	//    since we're describing the triangle in terms of the window itself
	Vertex vertices[] =
	{
		{ XMFLOAT3(+0.0f, +0.5f, +0.0f), red },
		{ XMFLOAT3(+0.5f, -0.5f, +0.0f), blue },
		{ XMFLOAT3(-0.5f, -0.5f, +0.0f), green },
	};

	// Set up indices, which tell us which vertices to use and in which order
	// - This is redundant for just 3 vertices, but will be more useful later
	// - Indices are technically not required if the vertices are in the buffer 
	//    in the correct order and each one will be used exactly once
	// - But just to see how it's done...
	unsigned int indices[] = { 0, 1, 2 };

	shapes[0] = std::make_shared<GameEntity>(std::make_shared<Mesh>(vertices, 3, indices, 3, context, device));

	Vertex vertices1[] =
	{
		{ XMFLOAT3(-0.5f + 1, +0.5f + 1, +0.0f), red },
		{ XMFLOAT3(+0.5f + 1, +0.5f + 1, +0.0f), blue },
		{ XMFLOAT3(+0.5f + 1, -0.5f + 1, +0.0f), red },
		{ XMFLOAT3(-0.5f + 1, -0.5f + 1, +0.0f), blue }
	};

	unsigned int indices1[] = { 0, 1, 2, 0, 2, 3 };

	shapes[1] = std::make_shared<GameEntity>(std::make_shared<Mesh>(vertices1, 4, indices1, 6, context, device));

	Vertex vertices2[] =
	{
		{ XMFLOAT3(+0.0f - 0.5, +0.3f + 0.5, +0.0f), red },
		{ XMFLOAT3(+0.3f - 0.5, +0.0f + 0.5, +0.0f), blue },
		{ XMFLOAT3(-0.3f - 0.5, +0.0f + 0.5, +0.0f), green },
		{ XMFLOAT3(+0.0f - 0.5, -0.3f + 0.5, +0.0f), magenta }
	};

	unsigned int indices2[] = { 2,0,1,2,1,3,2 };

	shapes[2] = std::make_shared<GameEntity>(std::make_shared<Mesh>(vertices2, 4, indices2, 7, context, device));

	//copied shapes
	shapes[3] = std::make_shared<GameEntity>(std::make_shared<Mesh>(vertices, 3, indices, 3, context, device));
	shapes[4] = std::make_shared<GameEntity>(std::make_shared<Mesh>(vertices1, 4, indices1, 6, context, device));
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	for (int i = 0; i < 3; i++) {
		camera[i]->UpdateProjectionMatrix((float)this->windowWidth / this->windowHeight);
	}
	// Handle base-level DX resize stuff
	DXCore::OnResize();
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	{
		// Feed fresh input data to ImGui
		ImGuiIO& io = ImGui::GetIO();
		io.DeltaTime = deltaTime;
		io.DisplaySize.x = (float)this->windowWidth;
		io.DisplaySize.y = (float)this->windowHeight;


		// Reset the frame
		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		// Determine new input capture
		Input& input = Input::GetInstance();
		input.SetKeyboardCapture(io.WantCaptureKeyboard);
		input.SetMouseCapture(io.WantCaptureMouse);

		// Show the demo window
		//ImGui::ShowDemoWindow(); 
		ImGui::Begin("Window");
		ImGui::Text("FPS: %f", io.Framerate);
		ImGui::Text("Window dimensions: %i x %i", windowWidth, windowHeight);
		for (int i = 0; i < 5; i++) {
			ImGui::PushID(i);
			if (ImGui::CollapsingHeader("Shape"))
			{

				if (ImGui::DragFloat3("Translation", translation[i])) {
					shapes[i]->GetTransform()->MoveAbsolute(XMFLOAT3(translation[i]));
				}
				if (ImGui::DragFloat3("Rotation", rotation[i])) {
					shapes[i]->GetTransform()->Rotate(XMFLOAT3(rotation[i]));
				}
				if (ImGui::DragFloat3("Scale", scale[i])) {
					shapes[i]->GetTransform()->Scale(XMFLOAT3(scale[i]));
				}
				if (ImGui::ColorEdit3("Color", colorOffset[i])) {
					shapes[i]->GetMesh()->SetTint(colorOffset[i][0], colorOffset[i][1], colorOffset[i][2], colorOffset[i][3]);
				}
			}
			ImGui::PopID();
		}
		if (ImGui::CollapsingHeader("Camera Settings")) {
			ImGui::Text("Camera %i x: %f y: %f z: %f",
				(activeCamera % 3) + 1,
				camera[activeCamera]->GetTransform()->GetPosition().x,
				camera[activeCamera]->GetTransform()->GetPosition().y,
				camera[activeCamera]->GetTransform()->GetPosition().z);
			ImGui::Text("FOV: %f Radians", camera[activeCamera]->GetFov());
			if (ImGui::Button("Change Camera")) {
				activeCamera = (activeCamera + 1) % 3;
			}
		}
		ImGui::End();
		if (input.KeyPress('C')) {
			activeCamera = (activeCamera + 1) % 3;
		}
	}

	{
		//translation
		if (shapes[0]->GetTransform()->GetPosition().x <= 1.0f && going) {
			shapes[0]->GetTransform()->MoveAbsolute(0.001f, 0.001f, 0.0f);
			shapes[4]->GetTransform()->MoveAbsolute(-0.001f, -0.001f, 0.0f);
		}
		else if (shapes[0]->GetTransform()->GetPosition().x > 0.0f)
		{
			going = false;
			shapes[0]->GetTransform()->MoveAbsolute(-0.001f, -0.001f, 0.0f);
			shapes[4]->GetTransform()->MoveAbsolute(0.001f, 0.001f, 0.0f);
		}
		else {
			going = true;
		}

		//scale
		if (!going) {
			shapes[2]->GetTransform()->Scale(0.999f, 0.999f, 1.0f);
		}
		else if (going) {
			shapes[2]->GetTransform()->Scale(1.001f, 1.001f, 1.0f);
		}

		//rotation
		shapes[3]->GetTransform()->Rotate(0.0f, 0.0f, deltaTime * XMConvertToRadians(10));
		shapes[1]->GetTransform()->Rotate(0.0f, 0.0f, deltaTime * XMConvertToRadians(10));
	}

	camera[activeCamera]->Update(deltaTime);

	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{

	// Frame START
	// - These things should happen ONCE PER FRAME
	// - At the beginning of Game::Draw() before drawing *anything*
	{
		// Clear the back buffer (erases what's on the screen)
		const float bgColor[4] = { 0.4f, 0.6f, 0.75f, 1.0f }; // Cornflower Blue
		context->ClearRenderTargetView(backBufferRTV.Get(), bgColor);

		// Clear the depth buffer (resets per-pixel occlusion information)
		context->ClearDepthStencilView(depthBufferDSV.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);
	}

	//Drawing shapes -A
	for (int i = 0; i < 5; i++) {
		shapes[i]->Draw(vsConstantBuffer, context, *camera[activeCamera]);
	}

	// Frame END
	// - These should happen exactly ONCE PER FRAME
	// - At the very end of the frame (after drawing *everything*)
	{
		// Present the back buffer to the user
		//  - Puts the results of what we've drawn onto the window
		//  - Without this, the user never sees anything
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		ImGui::Render();
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);

		// Must re-bind buffers after presenting, as they become unbound
		context->OMSetRenderTargets(1, backBufferRTV.GetAddressOf(), depthBufferDSV.Get());
	}
}