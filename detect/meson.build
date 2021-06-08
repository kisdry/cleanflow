project('prober', 'c')

dpdk = declare_dependency(
    dependencies:dependency('libdpdk'),
	        link_args:[
		 	'-lm',
		 	'-lpthread',
	        '-lyaml',
			'-lprotobuf-c',
			'-lrt'
		 	],
)
add_global_arguments('-DUSE_DPDK_MEM', language : 'c')
# add_global_arguments('-DLOG_LEVEL=LOG_LEVEL_INFO', language : 'c')

allow_experimental_apis = true

sources = files(
    'src/main.c',
)

incdir = include_directories('inc')

executable('prober', sources, dependencies: dpdk,include_directories:incdir)


