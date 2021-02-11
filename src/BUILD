cc_binary(
        name = 'graphize',
        srcs = glob([
            './*.cpp',
        ]),
        deps = [
        ':frame',
        ],
        #link_all_symbols=True
)

cc_library(
        name = 'frame',
        srcs = glob([
            #'frame/client/*.cpp',
            'frame/client/*/*.cpp',
            'frame/*.cpp',
            'common/*.cpp',
            'service/*.cpp',
            'service/base/*.cpp',

            # ut src
            'unittest/graph_frame/src/*.cpp',
            'unittest/redis_service/src/*.cpp',
        ]),
        deps = [
                '//thirdparty/brpc:brpc',
                '//thirdparty/shmcache:shmcache',
                '//thirdparty/fastcommon:fastcommon',
        ],
        hdrs=[],
)
cc_test(
    name = 'graph_ut',
    srcs = glob([
            #'unittest/test_main.cpp',
            # 'unittest/graph_frame/graph_test.cpp',
            'unittest/redis_service/redis_test.cpp',
    ]),
    deps = [
            ':frame',
            '//thirdparty/gtest:gtest',
    ],
    testdata = [
        "unittest/graph_frame/conf/node_service.conf",
        "unittest/graph_frame/conf/downstream.conf",
        "unittest/graph_frame/conf/graph.conf",
        "unittest/redis_service/conf/shmcache.conf",
        "unittest/redis_service/conf/node_service.conf",
        "unittest/redis_service/conf/downstream.conf",
        "unittest/redis_service/conf/graph.conf",
        "unittest/redis_service/conf/redis_client.conf",
    ],
    #link_all_symbols=True
)

#proto_library(
#    name = 'g_proto',
#    srcs = glob([
#    ]),
#)
