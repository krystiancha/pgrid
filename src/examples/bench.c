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
	static const size_t threads_ln = 6;
	static const size_t cache_ln = 5;
	static const char *input_path = "img/map.txt";
	static const float step = -0.02;
	static const size_t steps = 200;

	pthread_t threads[threads_ln];

	pgrid_grid_init(&grid, cache_ln);
	pgrid_grid_load(&grid, input_path, strlen(input_path));
	pgrid_threads_init(&grid, threads, threads_ln);

	assert(glfwInit());

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	GLFWwindow* window = glfwCreateWindow(width, height, "Pgrid", NULL, NULL);
	assert(window);

	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(0);

	pgrid_init(&pgrid, &grid, width, height, fov);
	pgrid.interp_scale = 0.5;

	for (size_t i = 0; i < steps; ++i) {
		float z = step * i;
		pgrid_render(&pgrid, (vec3) {0.4, 0, z}, (versor) {0, 0, 0, 1});
		glfwSwapBuffers(window);
	}

	pgrid_metrics_print(stdout, &pgrid, &grid);
	printf("\n");
	printf("Average velocity: %f m/s\n", fabs(step * steps)
		/ pgrid.metrics.frame_time);

	pgrid_finish(&pgrid);

	glfwDestroyWindow(window);
	glfwTerminate();

	pgrid_threads_finish(&grid, threads, threads_ln);
	pgrid_grid_finish(&grid);

	return 0;
}
