echo Install dependencies
echo Cinder-Hap2
git clone https://github.com/videodromm/Cinder-Hap2 ../../Cinder/blocks/Cinder-Hap2
echo Cinder-Videodromm
git clone https://github.com/videodromm/Cinder-Videodromm ../../Cinder/blocks/Cinder-Videodromm
echo Cinder-Videodromm-Textures
git clone https://github.com/videodromm/Cinder-Videodromm-Textures ../../Cinder/blocks/Cinder-Videodromm-Textures
echo Cinder-Videodromm-Fbos
git clone https://github.com/videodromm/Cinder-Videodromm-Fbos ../../Cinder/blocks/Cinder-Videodromm-Fbos
echo Cinder-Videodromm-UI
git clone https://github.com/videodromm/Cinder-Videodromm-UI ../../Cinder/blocks/Cinder-Videodromm-UI
echo Cinder-Videodromm-Router
git clone https://github.com/videodromm/Cinder-Videodromm-Router ../../Cinder/blocks/Cinder-Videodromm-Router
echo Cinder-Videodromm-Warps
git clone https://github.com/videodromm/Cinder-Videodromm-Warps ../../Cinder/blocks/Cinder-Videodromm-Warps
eecho Cinder-Warping
git clone https://github.com/paulhoux/Cinder-Warping ../../Cinder/blocks/Cinder-Warping
echo Cinder-ImGui
git clone https://github.com/simongeilfus/Cinder-ImGui ../../Cinder/blocks/Cinder-ImGui --recursive
echo Cinder-MIDI2
git clone https://github.com/brucelane/Cinder-MIDI2 ../../Cinder/blocks/Cinder-MIDI2
echo Cinder-WebSocketPP
git clone https://github.com/videodromm/Cinder-WebSocketPP ../../Cinder/blocks/Cinder-WebSocketPP
echo Cinder-WebSocketPP
git clone https://github.com/videodromm/assets ..
echo Required dependencies checked out
echo Pull changes
git pull
cd ../../Cinder/blocks/Cinder-Hap2
git pull
cd ../Cinder-Videodromm
git pull
cd ../Cinder-Videodromm-Textures
git pull
cd ../Cinder-Videodromm-Fbos
git pull
cd ../Cinder-Videodromm-UI
git pull
cd ../Cinder-Videodromm-Router
git pull


