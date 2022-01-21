# untitled

Batch title uninstaller for the Nintendo Switch

![Img](images/example0.jpg)

![Img](images/example1.jpg)

![Img](images/example2.jpg)

---

## Building

the following needs to be installed via dkp-pacman

- uam
- switch-glm
- libnx
- deko3d

```shell
sudo pacman -S uam switch-glm libnx deko3d
```

then download the repo using git (you can instead download the zip if you prefer)

```shell
git clone https://github.com/ITotalJustice/untitled.git
cd untitled
```

then build:

```shell
make -j
```
if you're having trouble building, feel free to open an issue!

---

## Credits

Special thank you to everyone that contributed to the following libs.

- [nanovg](https://github.com/memononen/nanovg)

- [libnx](https://github.com/switchbrew/libnx)

- [libnx for their default icon](https://github.com/switchbrew/libnx/blob/master/nx/default_icon.jpg)

- [deko3d by fincs](https://github.com/devkitPro/deko3d)

- [deko3d backend for nanovg by adubbz](https://github.com/Adubbz/nanovg-deko3d)

And thank you to following people that helped me out

- shchmue for RE'ing `nsCalculateApplicationOccupiedSize` struct for me!

- werwolv for helping me out with templates ^^

- TeJay for helping test the app and providing the screenshots
