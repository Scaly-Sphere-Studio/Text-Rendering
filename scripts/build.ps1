# Visual Studio parameters
$vcxproj    = Resolve-Path "..\*.vcxproj";
$vc_dir     = "C:\Program Files (x86)\Microsoft Visual Studio";
$vc_version = "2019";
$vc_edition = "Community";

# Prepare batch command to set Visual C++ env and build the project
$batch_cmd = "`"$vc_dir\$vc_version\$vc_edition\VC\Auxiliary\Build\vcvarsall.bat`" x64";
$batch_cmd += " && devenv $vcxproj /Build `"Debug|x86`"";
$batch_cmd += " && devenv $vcxproj /Build `"Release|x86`"";
$batch_cmd += " && devenv $vcxproj /Build `"Debug|x64`"";
$batch_cmd += " && devenv $vcxproj /Build `"Release|x64`"";

# Call batch command
CMD /c $batch_cmd
