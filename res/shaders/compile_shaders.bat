@echo off

for %%f in (*.*) do (
	if "%%~xf"==".vert" (
		glslc %%~dpnxf -o %%~dpnf_vert.spv
	)
	if "%%~xf"==".frag" (
		glslc %%~dpnxf -o %%~dpnf_frag.spv
	)
)