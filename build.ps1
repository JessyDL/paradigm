param (
		[Alias("G")][string]$GENERATOR = "MSVC",
    [string]$COMPILER = "VC++",
    [Alias("a")][string]$ARCHITECTURE = "x64",
    [Alias("c")][switch]$CLEAN = $false,
    [switch]$CLEAN_CMAKE = $false,
    [switch]$CLEAN_BUILD = $false,
    [string]$ROOT_DIRECTORY = $(get-location),
    [string]$BUILD_FOLDER = "$ROOT_DIRECTORY\builds",
    [string]$CMAKE_FOLDER = "$ROOT_DIRECTORY\project_files",
    [string]$VK_VERSION = "1.1.82.1",
    [string]$VK_ROOT = "",
    [Alias("u")][Alias("update")][switch]$CMAKE_UPDATE = $false,
    [switch]$VULKAN_STATIC = $false,
    [switch]$CUSTOM_ARGUMENTS = $false,
    [switch]$VERBOSE_LOGGING = $false,		
    [string]$CMAKE_PARAMS = ""
 )
 
# ---------------------------------------------------------------------------------------------------------------------
# validate the arguments
# ---------------------------------------------------------------------------------------------------------------------

if($GENERATOR -eq "MSVC" -or $GENERATOR -eq "Ninja")
{
	if($COMPILER -ne "VC++" -and $COMPILER -ne "CLang")
	{
		echo "ERROR: the [COMPILER] [${COMPILER}] is incorrect. The [GENERATOR] [${GENERATOR}] can only have one of the following compilers [VC++], [LLVM], or [CLang]"
		exit 1
	}
}
elseif($GENERATOR -eq "Make")
{
	if($COMPILER -ne "CLang")
	{
		echo "${PURPLE}WARNING: the [COMPILER] [${COMPILER}] is incorrect. Only [CLang] is supported for the [GENERATOR] [${GENERATOR}]. Setting the [COMPILER] to [CLang] now."
		$COMPILER="CLang"
	}
}
else
{ 
	echo "ERROR: did not recognize the [GENERATOR] [$GENERATOR], [GENERATOR] should be of the value [MSVC],[Ninja], or [Make]"
	exit 1
}

if( $ARCHITECTURE -ne "ARM" -and $ARCHITECTURE -ne "x64")
{
	echo "incorrect [ARCHITECTURE] set, please either set it to [ARM] or [x64]"
	exit 1
}

$cmake_output="$GENERATOR/$ARCHITECTURE".ToLower()
$build_output="$GENERATOR/$ARCHITECTURE".ToLower()

$CMAKE_PLATFORM=""
switch($GENERATOR)
{
	"MSVC" 
	{ 
		$CMAKE_PLATFORM=" -T v141 "
		if ($ARCHITECTURE -eq "x64" ) { $target_platform="Win64" }
		else { $target_platform="ARM" }
		$CMAKE_GENERATOR="Visual Studio 15 2017 $target_platform" 
		$cmake_output="${cmake_output}-VC++"
		$build_output="${build_output}-VC++"
	}
	"Make" { $CMAKE_GENERATOR="Unix Makefiles" }
	default { $CMAKE_GENERATOR=$GENERATOR }
}

if($CMAKE_UPDATE -and $CLEAN_CMAKE)
{
	echo "ERROR: conflicting arguments: both cmake_clean and update are enabled, but only one can be enabled at a time."
	exit 1
}


#todo validate cmake_update

if(!$CMAKE_UPDATE -and [string]::IsNullOrEmpty($CONFIG))
{
	if([string]::IsNullOrEmpty($VK_ROOT))
	{
		$VK_ROOT="C:/VulkanSDK/"
	}
	echo "the vulkan SDK root has been set to $VK_ROOT"
	if(!(Test-Path $VK_ROOT -PathType Container))
	{
		echo "ERROR: the automatic search for the vulkan SDK yielded no results, please pass in the value using the [VK_ROOT] argument."
		exit 1
	}
}

if(!(Test-Path "$VK_ROOT/$VK_VERSION" -PathType Container))
{
	echo "ERROR: could not find the vulkan SDK version [VK_VERSION] [${VK_VERSION}], please install it to continue or choose a different version."
	echo "    looked in $VK_ROOT/$VK_VERSION.."
	exit 1
}


# ---------------------------------------------------------------------------------------------------------------------
# generate the cmake project
# ---------------------------------------------------------------------------------------------------------------------
mkdir -p $CMAKE_FOLDER/$cmake_output -Force
mkdir -p $BUILD_FOLDER/$build_output -Force

if($CLEAN_BUILD -or $CLEAN)
{
	echo "cleaning $BUILD_FOLDER/$build_output"
	Get-ChildItem -Path "$BUILD_FOLDER/$build_output" -Recurse | Remove-Item -force -recurse
}

if($CLEAN_CMAKE -or $CLEAN)
{
	
	echo "cleaning $CMAKE_FOLDER/$cmake_output"
	Get-ChildItem -Path "$CMAKE_FOLDER/$cmake_output" -Recurse | Remove-Item -force -recurse
}

cd $CMAKE_FOLDER/$cmake_output

if($CMAKE_UPDATE)
{
	echo "reconfiguring project files for $CMAKE_GENERATOR"
	cmake . $CMAKE_PARAMS
}
elseif(!(Test-Path "CMakeFiles" -PathType Container))
{
	echo "generating project files for $CMAKE_GENERATOR"
	echo "setting root directory to $ROOT_DIRECTORY"
	echo "setting build directory to $BUILD_FOLDER/$build_output"
	$CMAKE_PARAMS="-DPE_BUILD_DIR=""$BUILD_FOLDER/$build_output"" -DVK_VERSION=""$VK_VERSION"" -DVK_ROOT=""$VK_ROOT"" -DVULKAN_STATIC=""$VULKAN_STATIC"" $CMAKE_PARAMS"
	echo "running cmake with the parameters $CMAKE_PARAMS "
	Start-Process cmake -ArgumentList "-G ""$CMAKE_GENERATOR"" ""$ROOT_DIRECTORY"" $CMAKE_PARAMS" -NoNewWindow
}

if($CUSTOM_ARGUMENTS -and $CMAKE_UPDATE)
{
	echo "${PURPLE}WARNING: custom arguments were sent, but there was already a cmake project in the location, ignoring the passed arguments."
	echo "$if this was unintended then you should pass the CLEAN/CLEAN_CMAKE command to regenerate the project with the new arguments"
}

# ---------------------------------------------------------------------------------------------------------------------
# build the project
# ---------------------------------------------------------------------------------------------------------------------

if(![string]::IsNullOrEmpty($CONFIG))
{
	echo "building project with configuration $CONFIG"
	cmake "--build . --config $CONFIG"
}

echo "completed building the project"
cd ../../../
exit 0