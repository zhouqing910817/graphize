cc_binary(
        name = 'graphize',
        srcs = glob([
            './*.cpp',
        ]),
        deps = [
        ':frame',
        ],
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
)

#proto_library(
#    name = 'g_proto',
#    srcs = glob([
#    ]),
#)
