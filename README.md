# Raspberry PI 4 Multiple Object Tracking(수정중)
## Client  

### ※ 필요 Library : OpenCV, Cmake  

### ※ 사용 언어 : C/C++  

### ※ Raspberry PI 4 구동환경에서 구현되었으며, 다른 버전에서는 구동되지 않습니다  
(다른 버전에서는 I/O Peripheral 시작 주소 및 오실레이터 주파수 변경이 필요)  
  
  
클라이언트 프로그램에서는 크게 두가지 기능을 수행합니다.  
첫번째로 영상을 실시간으로 촬영하고, 이를 UDP 소켓을 통해 서버로 전송하는 기능입니다.  
빠른 영상 전송을 위해 영상의 크기는 320x240 크기로 고정시켰으며, 이는 설정파일을 통해  
사용자가 직접 변경할 수 있습니다.  
<br>두번째로 UDP 소켓에서 명령어를 전달받아 기기의 모터 및 센서를 제어하는 기능입니다.  
라즈베리파이 기기에서는 디바이스 드라이버를 통해 외부 라이브러리 사용하지 않고  
구현하였으며, 직접 설계한 1Byte 명령어에 맞추어 기기의 센서와 모터를 제어합니다.  
모터 제어에 있어 방향 및 속도 조절을 위해 Breadboard에 H bridge 회로를 직접 구현했습니다.  
  
<img src="https://user-images.githubusercontent.com/78298368/208564705-7265b554-ed1b-4740-b946-42404c9ceded.png"  width="480" height="320"/>
(직접 구현한 H bridge 회로도)

사용법

1. Cmake 및 OpenCV 라이브러리 설치
2. module 폴더에서 make를 통해 커널 컴파일 및 모듈 등록
3. build 폴더에서 cmake 및 make를 통해 컴파일
4. build 폴더에서 main 파일 실행  
(상위 폴더에서 ini 설정파일 수정을 통해 ip 및 port 번호 수정)
### 전체적인 구동
