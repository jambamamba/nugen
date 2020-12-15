# edgetpu-minimal

[<img src="https://images-na.ssl-images-amazon.com/images/I/61Eautuke1L.__AC_SY300_SX300_QL70_FMwebp_.jpg"/>](https://github.com/jambamamba/leila.docker)

A minimum executable that loads libedgetpu library and is able to check the version of the [Coral Edge TPU](https://coral.ai/products/accelerator/) attached to a USB port.

### Requirement

Build and install the [leila.docker image](https://github.com/jambamamba/leila.docker). Then run the container:

```bash
./enterdocker.sh
```
password is dev

### Build

After you enter the docker container, clone this repo and run the build.sh script:

```bash
cd edgetpu-minimal 
./build.sh
```

### Run

If all goes well, the build.sh will generate the executable in the build folder. You can run it like this:

```bash
./build/minimal
```


