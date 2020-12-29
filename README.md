# edgetpu-minimal

<img src="res/IMG_8633.inferenced.JPG"/>

A minimum executable that loads libedgetpu library and is able to check the version of the [Coral Edge TPU](https://coral.ai/products/accelerator/) attached to a USB port.

Also minimal code needed to classify images and detect objects in images.

We use the Google Coal Edget TPU USB accelerator on a Raspberry Pi Zero

[<img style="width:20%;" src="https://lh3.googleusercontent.com/vvBAqSnXyg3h9yS0JLyVehhV-e__3NFbZ6q7Ft-rEZp-9wDTVZ49yjuYJwfa4jQZ-RVnChHMr-DDC0T_fTxVyQg3iBMD-icMQooD6A=w2000-rw"/>](https://coral.ai/products/accelerator)
[<img style="width:20%;" src="https://www.raspberrypi.org/homepage-9df4b/static/1dfa03d09c1f3e446e8d936dfb92267f/ae23f/6b0defdbbf40792b64159ab8169d97162c380b2c_raspberry-pi-zero-1-1755x1080.jpg"/>](https://www.raspberrypi.org/products/raspberry-pi-zero/)

### Requirement

Build and install the [docker.leila image](https://github.com/jambamamba/docker.leila). Then run the container:

```bash
git clone https://github.com/jambamamba/docker.leila.git
cd docker.leila
./build.sh
./enterdocker.sh
```
password is dev

### Build

After you enter the docker container, clone this repo and run the build.sh script:

```bash
git clone https://github.com/jambamamba/edgetpu-minimal.git
cd edgetpu-minimal 
./build.sh 
```

### Run

If all goes well, the build.sh will generate the executable in the build folder. You can run it like this:

```bash
./build/minimal
```
If you have the Coral USB accelerator plugged in, you should see this:

```bash
tf-docker ~/oosman/repos/edgetpu-minimal > ./build/minimal 
available_tpus.size: 1
INFO: Initialized TensorFlow Lite runtime.
Done
```

### Cross Compile for Raspberry Pi Zero

First build the Raspberry Pi Zero toolchain

```bash
git clone https://github.com/jambamamba/crosstool-ng.git
cd crosstool-ng
git checkout raspi0
./build.sh
```

This can take some time, go take a long shower meanwhile, you haven't done it for a while, and you don't smell so good!

```bash
cd edgetpu-minimal 
./build.sh arch=rpi
```

### Clean builds

```bash
./build.sh clean=true arch=host
./build.sh clean=true arch=rpi
```

### Object Classification

```bash
./build/classify cat224x224.rgb
```

The input file (cat.rgb) must be 224 x 224 raw RGB image.

You can use the convert utility to make RGB file like this:
```bash
convert cat.bmp -resize 224x224! cat.rgb
```

#### Output:
```bash
available_tpus.size: 1
INFO: Initialized TensorFlow Lite runtime.
Inferenced class "Persian cat" in 12 milliseconds
```

### Object Detection

```bash
./build/detect /path/to/cat.jpg
```

Note that you can pass in any size jpg or png file as input.

#### Output:
```bash
available_tpus.size: 1
INFO: Initialized TensorFlow Lite runtime.
Inferenced class "cat" at (1,14)=> (296,227) in 30 milliseconds
```

An output file is generated with bounding boxes around the detected objects.

