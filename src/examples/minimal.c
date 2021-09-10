#define GLFW_INCLUDE_NONE

#include <GLFW/glfw3.h>
#include <assert.h>
#include <string.h>
#include <math.h>

#include "glad/gl.h"
#include "pgrid/pgrid.h"

struct pgrid_grid grid;
struct pgrid pgrid;

int
main(void)
{
	static const int width = 1280;
	static const int height = 720;
	static const float fov = M_PI_2;
	static const size_t threads_ln = 1;
	static const size_t cache_ln = 1;
	static const char *input_path = "img/map.txt";
	vec3 pos = {0, 0, 0};
	versor rot = {0, 0, 0, 1};

	pthread_t threads[threads_ln];

	pgrid_grid_init(&grid, cache_ln);
	assert(pgrid_grid_load(&grid, input_path, strlen(input_path)));
	pgrid_threads_init(&grid, threads, threads_ln);

	assert(glfwInit());

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	GLFWwindow* window = glfwCreateWindow(width, height, "Pgrid", NULL, NULL);
	assert(window);

	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);

	pgrid_init(&pgrid, &grid, width, height, fov);

	while (!glfwWindowShouldClose(window)) {
		pgrid_render(&pgrid, pos, rot);
		glfwSwapBuffers(window);
		glfwWaitEvents();
	}

	pgrid_finish(&pgrid);

	glfwDestroyWindow(window);
	glfwTerminate();

	pgrid_threads_finish(&grid, threads, threads_ln);
	pgrid_grid_finish(&grid);

	return 0;
}
