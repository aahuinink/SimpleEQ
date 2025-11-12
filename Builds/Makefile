# Top-level makefile to generate both Windows and Linux builds from WSL

#	Variables
PROJECT_NAME = SimpleEQLinux
LINUX_MAKEFILE_DIR = LinuxMakefile
export WINDOWS_SLN_PATH = "$(subst \,\\,$(shell wslpath -w "$(shell pwd)"))\\VisualStudio2022\\$(PROJECT_NAME).sln"
export VS_DEV_CMD_SCRIPT = "C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\Tools\\VsDevCmd.bat"

# Default configuration is debug
CONFIG = Debug
export CONFIG

windows:
	@echo "Building Solution located in $(WINDOWS_SLN_PATH)"
	cmd.exe /C "call $(VS_DEV_CMD_SCRIPT) && msbuild.exe $(WINDOWS_SLN_PATH) /p:Configuration=$(CONFIG)"

linux:
	bear -- $(MAKE) -C $(LINUX_MAKEFILE_DIR)

clean:
	bear -- $(MAKE) clean -C $(LINUX_MAKEFILE_DIR)
