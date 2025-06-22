//#define STBI_NO_SIMD // Removes SIMD Support
// SIMD greatly improves the speed of your game
#if defined(__arm__)||(__aarch64__)

// Use Advance SIMD NEON when loading images for STB Default is SSE2 (x86)
#define STBI_NEON

#endif

#define OLC_GFX_OPENGL33
#define OLC_PGE_APPLICATION
#define OLC_IMAGE_STB
#include "olcUTIL_Hardware3D.h"
#include "olcPixelGameEngine.h"

// Override base class with your custom functionality
class PGE3DMountains : public olc::PixelGameEngine
{
public:
    PGE3DMountains()
    {
        // Name your application
        sAppName = "PGE 2.0 3D Mountains Example";
    }

    olc::mf4d matWorld;
    olc::mf4d matView;
    olc::mf4d matProject;
    olc::utils::hw3d::mesh meshMountain;
    olc::Renderable gfx1;

    olc::vf3d vf3dUp = { 0.0f, 1.0f, 0.0f };                // vf3d up direction
    olc::vf3d vf3dCamera = { -5.0f, 10.5f, -10.0f };         // vf3d camera direction
    olc::vf3d vf3dLookDir = { 0.0f, 0.0f, 1.0f };           // vf3d look direction
    olc::vf3d vf3dForward = { 0.0f, 0.0f, 0.0f };           // vf3d Forward direction
    olc::vf3d vf3dOffset = { -5.0f, 10.5f, -10.0f };         // vf3d Offset
    olc::vf3d vf3dSunLocation = { 100.0f, 100.0f, 100.0f }; // vf3d Sun Location

    float fYaw = 0.0f;            // FPS Camera rotation in X plane
    float fYawRoC = 1.0f;        // fYaw Rate of Change Look Up/Down
    float fTheta = 0.0f;        // Spins World transform
    float fThetaRoC = 1.5f;        // fTheta Rate of Change Spin Left/Right
    float fStrifeRoC = 8.5f;    // Strife Rate of Change, thanks: #Boguslavv
    float fForwardRoC = 8.0f;   // Forward/Backwards Rate of Change

    float fSphereRoC = 0.5f;    // Sphere Rate of Change
    float fSphereRotationY = -1.57079633; // Sphere start Y rotation position

    float fJump = vf3dOffset.y;     // Monitors jump height so we can land again
    float fJumpRoC = 4.0f;    // fTheta Rate of Change


    /* Vectors */
    std::vector<std::string> vecMessages;
    /* END Vectors*/

    uint32_t nFrameCount = 0;
    float fStep = 20.0f;
    olc::vf2d vf2MessPos = { 10.0f, 10.0f };

    /* Sprites */
    olc::Sprite* sprTouchTester = nullptr;
    olc::Sprite* sprOLCPGEMobLogo = nullptr;
    olc::Sprite* sprLandScape = nullptr;
    /* END Sprites*/

    /* Decals */
    olc::Decal* decTouchTester = nullptr;
    olc::Decal* decOLCPGEMobLogo = nullptr;
    olc::Decal* decLandScape = nullptr;
    /* End Decals */

    /* Renders */
    olc::Renderable renTestCube;
    olc::Renderable renBrick;
    olc::Renderable renEarth;
    olc::Renderable renSkyCube;
    /* End Renders */




// 3D Camera
    olc::utils::hw3d::Camera3D Cam3D;

    // Sanity Cube
    olc::utils::hw3d::mesh matSanityCube;
    olc::utils::hw3d::mesh matSkyCube;
    olc::utils::hw3d::mesh matTriangle;
    olc::utils::hw3d::mesh matPyramid;
    olc::utils::hw3d::mesh mat4SPyramid;
    olc::utils::hw3d::mesh matSphere;
    
    // Manage Touch points
    olc::vi2d centreScreenPos;
    olc::vi2d leftCenterScreenPos;
    olc::vi2d rightCenterScreenPos;


public:
    bool OnUserCreate() override
    {
        float fAspect = float(GetScreenSize().x) / float(GetScreenSize().y); // Width / height
        float S = 1.0f / (tan(3.14159f * 0.25f));
        float f = 1000.0f;
        float n = 0.1f;


        matWorld.identity();
        matView.identity();

        Cam3D.SetScreenSize(GetScreenSize()); // SetAspectRatio(fAspect);
        Cam3D.SetClippingPlanes(n, f);
        Cam3D.SetFieldOfView(S);


        auto t = olc::utils::hw3d::LoadObj("./assets/objectfiles/mountains.obj");
        if (t.has_value())
        {
            meshMountain = *t;
        }
        else
        {
            int pause = 0; // TODO: Remove. We have an issue
        }
        
        Clear(olc::BLUE);

       sprTouchTester = new olc::Sprite("./assets/images/north_south_east_west_logo.png");
       decTouchTester = new olc::Decal(sprTouchTester);

       sprOLCPGEMobLogo = new olc::Sprite("./assets/images/olcpgemobilelogo.png");
       decOLCPGEMobLogo = new olc::Decal(sprOLCPGEMobLogo);

       // TODO: Change this to a renederable
       sprLandScape = new olc::Sprite("./assets/images/MountainTest1.jpg");
       decLandScape = new olc::Decal(sprLandScape);

      
        centreScreenPos = GetScreenSize();
        centreScreenPos.x = centreScreenPos.x / 2;
        centreScreenPos.y = centreScreenPos.y / 2;

        // Called once at the start, so create things here
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override
    {
        SetDrawTarget(nullptr);
       Clear(olc::BLUE);
       // New code:
       olc::vf3d  vf3Target = {0,0,1};

       olc::mf4d mRotationX, mRotationY, mRotationZ;  // Rotation Matrices
       olc::mf4d mPosition, mCollision;                // Position and Collision Matrices
       olc::mf4d mMovement, mOffset;                   // Movement and Offset Matrices

       // Update our camera position first, as this is what everything else is base upon
       // Create a "Point At"
       olc::vf3d vf3dTarget = { 0,0,1 };

       mRotationY.rotateY(fTheta);  // Left/Right
       mRotationX.rotateX(fYaw);    // Up/Down

       vf3dLookDir = mRotationY * mRotationX * vf3dTarget;   // Left-Right * Up-Down
       vf3dTarget = vf3dCamera + vf3dLookDir;

       Cam3D.SetPosition(vf3dCamera);
       Cam3D.SetTarget(vf3dTarget);
       Cam3D.Update();
       matWorld = Cam3D.GetViewMatrix();

       // Manage forward / backwards
       vf3dForward = vf3dLookDir * (fForwardRoC * fElapsedTime);

       ClearBuffer(olc::CYAN, true); // Clear the buffer folks


       HW3D_Projection(Cam3D.GetProjectionMatrix().m);

       // Lighting
       for (size_t i = 0; i < meshMountain.pos.size(); i += 3)
       {
           const auto& p0 = meshMountain.pos[i + 0];
           const auto& p1 = meshMountain.pos[i + 1];
           const auto& p2 = meshMountain.pos[i + 2];

           olc::vf3d vCross = olc::vf3d(p1[0] - p0[0], p1[1] - p0[1], p1[2] - p0[2]).cross(olc::vf3d(p2[0] - p0[0], p2[1] - p0[1], p2[2] - p0[2])).norm();

           olc::vf3d vLight = olc::vf3d(1.0f, 1.0f, 1.0f).norm();

           float illum = std::clamp(vCross.dot(vLight), 0.0f, 1.0f) * 0.6f + 0.4f;
           meshMountain.col[i + 0] = olc::PixelF(illum, illum, illum, 1.0f);
           meshMountain.col[i + 1] = olc::PixelF(illum, illum, illum, 1.0f);
           meshMountain.col[i + 2] = olc::PixelF(illum, illum, illum, 1.0f);
       }

       // Draw a line
       HW3D_DrawLine((matWorld).m, { 0.0f, 0.0f, 0.0f }, { 100.0f, 100.0f, 100.0f }, olc::RED);

       // Draw a Box
       HW3D_DrawLineBox((matWorld).m, { 0.0f, 0.0f, 0.0f }, { 10.0f, 10.0f, 10.0f }, olc::YELLOW);

       // Draw the world
       HW3D_DrawObject((matWorld).m, decLandScape, meshMountain.layout, meshMountain.pos, meshMountain.uv, meshMountain.col);

       // End new code

       UpdateCamByUserInput(fElapsedTime);

       DisplayMessages();

       // Draw Logo
       DrawDecal({ 5.0f, (float)ScreenHeight() - 100 }, decOLCPGEMobLogo, { 0.5f, 0.5f });

        if (GetKey(olc::Key::ESCAPE).bPressed)
        {
            return false;
        }
        else
        {
            return true;
        }

       
       
        
    }

    /*
    * Updates the cam position by user input
    * Mouse/Touch/Keyboard
    */
    void UpdateCamByUserInput(float fElapsedTime)
    {
        // Handle Camera
        // Touch zeros (single touch) handles Camera look direction
        if (GetMouse(0).bHeld)
        {

            // We know the Right Center point we need to compare our positions
            // Looking Right
            if ((float)GetMousePos().x > (((float)centreScreenPos.x / 100) * 130))
            {
                fTheta -= fThetaRoC * fElapsedTime;


            }

            // Looking Left
            if ((float)GetMousePos().x < (((float)centreScreenPos.x / 100) * 70))
            {
                fTheta += fThetaRoC * fElapsedTime;


            }

            // Looking Up
            if ((float)GetMousePos().y < (((float)centreScreenPos.y / 100) * 70))
            {
                fYaw -= fYawRoC * fElapsedTime;
                if (fYaw < -1.0f) fYaw = -1.0f;
            }

            // Looking Down
            if ((float)GetMousePos().y > (((float)centreScreenPos.y / 100) * 130))
            {
                fYaw += fYawRoC * fElapsedTime;
                if (fYaw > 1.0f) fYaw = 1.0f;
            }

        }
        else
        {
            // Move the camera back to center, stops the dizzies!
            if (fYaw > -0.01f && fYaw < 0.01f)
            {
                fYaw = 0.0f;
            }
            if (fYaw >= 0.01)
            {
                fYaw -= fYawRoC * fElapsedTime;

            }
            if (fYaw <= -0.01)
            {
                fYaw += fYawRoC * fElapsedTime;

            }

        }

        // Handle movement
        // Moving Forward
        if (GetKey(olc::Key::UP).bHeld || GetMouse(1).bHeld)
        {
            vf3dCamera += vf3dForward;
        }

        // Moving Backward
        if (GetKey(olc::Key::DOWN).bHeld)
        {
            vf3dCamera -= vf3dForward;
        }

        // Moving Left (Strife)
        if (GetKey(olc::Key::LEFT).bHeld)
        {
            vf3dCamera.x -= cos(fTheta) * fStrifeRoC * fElapsedTime;
            vf3dCamera.z -= sin(fTheta) * fStrifeRoC * fElapsedTime;
        }


        // Moving Right (Strife)
        if (GetKey(olc::Key::RIGHT).bHeld)
        {
            vf3dCamera.x += cos(fTheta) * fStrifeRoC * fElapsedTime;
            vf3dCamera.z += sin(fTheta) * fStrifeRoC * fElapsedTime;

        }


        // Moving UP
        if (GetKey(olc::Key::SPACE).bHeld)
        {
            fJump += fJumpRoC * fElapsedTime;
            vf3dCamera.y = fJump;
        }
        else if (GetKey(olc::Key::B).bHeld)
        {
            fJump -= fJumpRoC * fElapsedTime;
            vf3dCamera.y = fJump;

        }
        else
        {
           /* if (fJump > (vf3dOffset.y - 0.01f) && fJump < (vf3dOffset.y + 0.01f))
            {
                fJump = vf3dOffset.y;
                vf3dCamera.y = fJump;
            }
            if (fJump >= (vf3dOffset.y + 0.01))
            {
                fJump -= 4.0f * fElapsedTime;
                vf3dCamera.y = fJump;
            }
            if (fJump <= (vf3dOffset.y - 0.01))
            {
                fJump += 4.0f * fElapsedTime;
                vf3dCamera.y = fJump;
            }*/
        }



        // Set Sun Location
        if (GetKey(olc::Key::S).bHeld)
        {
            vf3dSunLocation.x = float(GetMouseX());
            vf3dSunLocation.y = float(GetMouseY());
        }
    }

    /*
    * Displays messages on the screen
    */
    void DisplayMessages()
    {
        nFrameCount = GetFPS();

        std::string sMessage = "OneLoneCoder.com";
        vecMessages.push_back(sMessage);

        sMessage = sAppName + " - FPS: " + std::to_string(nFrameCount);
        vecMessages.push_back(sMessage);

        sMessage = "Sun X: " + std::to_string(vf3dSunLocation.x);
        vecMessages.push_back(sMessage);
        sMessage = "Sun Y: " + std::to_string(vf3dSunLocation.y);
        vecMessages.push_back(sMessage);
        sMessage = "Sun Z: " + std::to_string(vf3dSunLocation.z);
        vecMessages.push_back(sMessage);


        sMessage = "---";
        vecMessages.push_back(sMessage);

        fStep = 10;
        vf2MessPos.y = fStep;
        for (auto& s : vecMessages)
        {
            DrawStringDecal(vf2MessPos, s);
            vf2MessPos.y += fStep;
        }
        vecMessages.clear();


    }
};

int main(int argc, char const *argv[]) {
    PGE3DMountains demo;

        // Lets do HD!
        if (demo.Construct(1280, 720, 1, 1, false))
            demo.Start();
        return 0;
}
