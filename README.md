# Pgrid

Pgrid (formerly panoramagrid) is a library for rendering regular images from a
set of spherical images.
It was designed with photorealistic environment simulation for service robotics
in mind.
It can be thought of as a virtual camera that can be moved and rotated.

Check out
[this blog post](https://krystianch.com/taking-404-spherical-photos-in-2-hours/).

![Screenshots](https://files.krystianch.com/pgrid-screenshots.jpg)

## Requirements

* glfw ~3.3.4 [[website]](https://www.glfw.org/) [[alpine: `glfw-dev` or `glfw-wayland-dev`]](https://pkgs.alpinelinux.org/packages?name=glfw*-dev&branch=edge)
* cglm ~0.8.3 [[website]](http://cglm.readthedocs.io/) [[alpine: `cglm-dev`]](https://pkgs.alpinelinux.org/packages?name=cglm-dev&branch=edge)
* libjpeg-turbo ~2.1.0 [[website]](https://libjpeg-turbo.org/) [[alpine: `libjpeg-turbo-dev`]](https://pkgs.alpinelinux.org/packages?name=libjpeg-turbo-dev&branch=edge)
* meson 0.58.1 [[website]](https://mesonbuild.com) [[alpine: `meson`]](https://pkgs.alpinelinux.org/packages?name=meson&branch=edge)
* ninja 1.9 [[website]](https://github.com/michaelforney/samurai) [[alpine: `samurai`]](https://pkgs.alpinelinux.org/packages?name=samurai&branch=edge)

## Compiling

```sh
meson setup build
meson compile -C build
```

## Sample image set

A sample image set is provided at the link below.
It contains images covering part of Lab 012 @ Faculty of Electronics and
Information Technology (Warsaw University of Technology).
It contains 404 images taken on a 20 cm grid.

This work © 2019 by Krystian Chachuła and Maciej Stefańczyk is licensed under
[CC BY-SA 4.0](http://creativecommons.org/licenses/by-sa/4.0/)

https://files.krystianch.com/pgrid-feit-012-v1.tar.gz (1.6 GB)

## Running the demo

Download the sample image set (see previous section), extract it to `img/` and
run pgrid.

```sh
curl -o pgrid-feit-012-v1.tar.gz https://files.krystianch.com/pgrid-feit-012-v1.tar.gz
tar -xzf pgrid-feit-012-v1.tar.gz

build/pgrid img/map.txt
```

For more options see:

```sh
build/pgrid -h
```

## Spherical image set

A set of spherical images, that is required for full functionality of the
library, consists of the map file and images.
The map file is a text file that puts spherical images in 3D space.
Its syntax is as follows.

```text
img1.jpg 4.2 0.0 0.0
img2.jpg 4.2 0.0 0.2
img3.jpg 4.2 0.0 0.4
```

For an example image set see section [Sample image set] above.

## Using the library

TL;DR: There are two example programs. Check them out: 
[minimal example](src/examples/minimal.c), [benchmark](src/examples/bench.c)

The interface consists of two components.
The grid and the renderer.
The grid is a container for spherical images with metadata.
It loads and decodes the images on demand (currently limited to JPEG).
It can work in single mode for uses where the position of the virtual camera is
constant or in grid mode otherwise.

### Single mode

In single mode only a single spherical image is used.
This way different perspectives can be rendered from a single spherical image,
but only from the same position.
This means only orientation of the camera can change.

In single mode the grid should be set up like this:

```c
struct pgrid_grid grid;

pgrid_grid_init(&grid, 5);
pgrid_grid_single(&grid, input_path, strlen(input_path));

/* ... */

pgrid_grid_finish(&grid);
```

`input_path` is the path to the single spherical image.

### Grid mode

Grid mode unleashes the full power of Pgrid.
It requires multiple spherical images and a map file as described above.

In grid mode the grid should be set up like this:

```c
struct pgrid_grid grid;
pthread_t threads[threads_ln];

pgrid_grid_init(&grid, 5);
pgrid_grid_load(&grid, input_path, strlen(input_path));
pgrid_threads_init(&grid, threads, threads_ln);

/* ... */

pgrid_threads_finish(&grid, threads, threads_ln);
pgrid_grid_finish(&grid);
```

### Renderer

After setting up the grid the renderer has to be set up before images can be
rendered.
In order to initialize and use the renderer there has to be a *current* OpenGL
context.
The GLFW library can help with this. (see
[src/examples/minimal.c](src/examples/minimal.c))

The renderer should be initialized like this:

```c

struct pgrid pgrid;

pgrid_init(&pgrid, &grid, (float) width / (float) height, false);

/* ... */

pgrid_finish(&pgrid);
```

After the renderer is initialized images can be rendered by specifying the
position and orientation of the camera.

```c
pgrid_render(&pgrid, pos, rot);
```

## Profiling

```sh
perf record --event cycles --no-inherit --call-graph lbr -- build/bench
perf report
```

## Contributing

This code uses
[Drew DeVault's C Style Guide](https://git.sr.ht/~sircmpwn/cstyle).

Send patches to
[~krystianch/public-inbox@lists.sr.ht](mailto:~krystianch/public-inbox@lists.sr.ht).
([help, I only know pull requests](https://git-send-email.io/))

## Papers

The utilization of spherical camera in simulation for service robotics

Krystian Chachuła and Maciej Stefańczyk

2021 2103.09297 arXiv cs.RO

https://arxiv.org/abs/2103.09297

## Acknowledgements

I'd like to thank Maciej Stefańczyk, who came up with the idea for this project,
for supervision, valuable comments, and help with creating the image set.

## License

[GNU General Public License](LICENSE)
