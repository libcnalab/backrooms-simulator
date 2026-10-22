#include "BackroomsGame.hpp"

#include <cstdint>
#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    try {
        std::uint64_t seed = 0xBACC0005ULL;
        bool streamTest=false;
        int level=0, multiSampleCount=4, streamTestLoops=1;
        double x=2.5,z=2.5;
        double walkSpeed=Backrooms::BackroomsGame::kDefaultWalkSpeed;
        double runSpeed=Backrooms::BackroomsGame::kDefaultRunSpeed;
        double streamTestMetres=9600.0;
        float verticalFov=Backrooms::BackroomsGame::kDefaultVerticalFov;
        std::string capture;
        float captureTime=0;
        bool view=false;
        bool shadows=true;
        float yaw=0,pitch=0;
        for (int i=1;i<argc;++i) {
            const std::string argument=argv[i];
            if (argument=="--seed" && i+1<argc)
                seed=std::stoull(argv[++i],nullptr,0);
            else if (argument=="--level" && i+1<argc)
                level=std::stoi(argv[++i]);
            else if (argument=="--position" && i+2<argc) {
                x=std::stod(argv[++i]);
                z=std::stod(argv[++i]);
            }
            else if (argument=="--walk-speed" && i+1<argc)
                walkSpeed=std::stod(argv[++i]);
            else if (argument=="--run-speed" && i+1<argc)
                runSpeed=std::stod(argv[++i]);
            else if (argument=="--fov" && i+1<argc)
                verticalFov=std::stof(argv[++i]);
            else if (argument=="--msaa" && i+1<argc)
                multiSampleCount=std::stoi(argv[++i]);
            else if (argument=="--stream-test") streamTest=true;
            else if (argument=="--stream-test-metres" && i+1<argc) {
                streamTest=true;
                streamTestMetres=std::stod(argv[++i]);
            }
            else if (argument=="--stream-test-loops" && i+1<argc) {
                streamTest=true;
                streamTestLoops=std::stoi(argv[++i]);
            }
            else if (argument=="--view" && i+2<argc) {
                view=true;
                yaw=std::stof(argv[++i]);
                pitch=std::stof(argv[++i]);
            }
            else if (argument=="--capture" && i+1<argc)
                capture=argv[++i];
            else if (argument=="--time" && i+1<argc)
                captureTime=std::stof(argv[++i]);
            else if (argument=="--no-shadows") shadows=false;
            else {
                std::cerr << "Usage: cna_backrooms [--seed unsigned-integer] "
                             "[--level 0|1|2] [--position x z] [--view yaw pitch] "
                             "[--walk-speed m/s] [--run-speed m/s] [--fov degrees] "
                             "[--msaa 0|4] [--stream-test] [--stream-test-metres distance] "
                             "[--stream-test-loops 1..8] [--capture file.png [--time seconds]] "
                             "[--no-shadows]\n";
                return 2;
            }
        }
        Backrooms::BackroomsGame game(seed,streamTest,level,x,z,
                                      walkSpeed,runSpeed,streamTestMetres,verticalFov,
                                      multiSampleCount,streamTestLoops);
        if (view) game.SetView(yaw,pitch);
        if (!shadows) game.SetShadows(false);
        if (!capture.empty()) game.RequestCapture(capture,captureTime);
        game.Run();
    } catch (const std::exception& e) {
        std::cerr << "cna-backrooms: " << e.what() << '\n';
        return 1;
    }
}
