project('cleanflow', 'c')

dpdk = declare_dependency(
    dependencies:dependency('libdpdk'),
	        link_args:[
		 	'-lm',
		 	'-lpthread',
	        	'-lyaml',
			'-lprotobuf-c',
		 	],
)
add_global_arguments('-DUSE_DPDK_MEM', language : 'c')

allow_experimental_apis = true

sources = files(
    'cleanflow.c',
)

executable('cleanflow', sources, dependencies: dpdk)


