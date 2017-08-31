{
    "targets": [{
        "target_name": "stream",
        "sources": [ "./stream.cpp" ],
		'conditions': [
			['OS=="mac"', {
				'include_dirs' : [
				],
				'libraries': [
				],
				'cflags': [
					"-g"
				]
			},
			'OS=="win"', {
				'include_dirs' : [
				],
				'libraries': [
				],
				'cflags': [
					"-g"
				]
			},
			'OS=="linux"', {
				'include_dirs' : [
				],
				'libraries': [
				],
				'cflags': [
					"-g"
				]
			}
			]
		]
    }]
}
