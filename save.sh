UNAMEOUT="$(uname -s)"

case "${UNAMEOUT}" in
    Linux*)     MACHINE=Linux;;
    Darwin*)    MACHINE=Mac;;
    CYGWIN*)    MACHINE=Cygwin;;
    MINGW*)     MACHINE=MinGw;;
    *)          MACHINE="UNKNOWN:${unameOut}"
esac

if [ -f "melodeergui" ]
then

	if [ ! -d "exec" ]
	then
		mkdir "exec"
	fi

	if [ ! -d "exec/$MACHINE" ]
	then
		mkdir "exec/$MACHINE"
	fi

	cp "melodeergui" "exec/$MACHINE/"
fi
