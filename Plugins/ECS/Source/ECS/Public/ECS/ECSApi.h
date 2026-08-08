#pragma once

#if defined(_WIN32) || defined(_WIN64)
	#ifdef MAHO_ECS_MODULE_EXPORTS
		#define MAHO_ECS_API __declspec(dllexport)
	#else
		#define MAHO_ECS_API __declspec(dllimport)
	#endif
#else
	#define MAHO_ECS_API
#endif
