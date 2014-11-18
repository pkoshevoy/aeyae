#!/bin/bash -x
# -*- Mode: shell-script; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: t -*-

#----------------------------------------------------------------
# BUNDLE_PATH
#
BUNDLE_PATH=${1}

#----------------------------------------------------------------
# CP
#
# Use rsync because it's much faster than 'cp -r'.
#
CP='rsync -a --copy-unsafe-links --chmod=ug+rwX'

#----------------------------------------------------------------
# DARWIN_REV
#
DARWIN_REV=`uname -r`

#----------------------------------------------------------------
# INSTALL_NAME_TOOL=
# 
INSTALL_NAME_TOOL=`which install_name_tool`
if [ -x '/opt/local/bin/install_name_tool' ]; then
	INSTALL_NAME_TOOL='/opt/local/bin/install_name_tool'
fi

#----------------------------------------------------------------
# XCODEBUILD
#
XCODEBUILD='xcodebuild'
if [ "${DARWIN_REV}" = "8.11.0" -o "${DARWIN_REV}" = "8.11.1" ]; then
	XCODE_SDK='/Developer/SDKs/MacOSX10.4u.sdk'
	BUILDING_FOR=Tiger
elif [ "${DARWIN_REV}" = "9.8.0" -o "${DARWIN_REV}" = "9.9.1" ]; then
	XCODE_SDK='/Developer/SDKs/MacOSX10.5.sdk'
	BUILDING_FOR=Leopard
else # snow leopard?
	XCODE_SDK='/Developer/SDKs/MacOSX10.6.sdk'
	BUILDING_FOR=SnowLeopard
fi


#----------------------------------------------------------------
# quiet_pushd
#
# $1 -- directory to push
quiet_pushd()
{
	DIR=${1}
	if [ -z "${1}" ]; then exit 10; fi

	pushd "${DIR}" 1>/dev/null 2>/dev/null
	if [ $? != 0 ]; then
		echo ERROR: failed to change current directory to "${DIR}"
		exit 1;
	fi
}

#----------------------------------------------------------------
# quiet_popd
#
quiet_popd()
{
	popd 1>/dev/null 2>/dev/null
}

#----------------------------------------------------------------
# replace_all
#
# $1 -- filename
# $2 -- search string
# $3 -- replacement string
replace_all()
{
	FN="${1}"
	SRCH="${2}"
	RPLC="${3}"

	if [ -z "{FN}" -o -z "${SRCH}" ]; then
		echo USAGE: ${0} filename search_string replacement_string
		exit 3;
	fi

	if [ ! -f "${FN}" ]; then
		echo ERROR: can not read \'"${FN}"\'
		exit 1;
	fi

	touch "${FN}".tmp
	if [ ! -f "${FN}".tmp ]; then
		echo ERROR: can not write to \'"${FN}".tmp\'
		exit 2;
	fi

	sed -e 's/'"${SRCH}"'/'"${RPLC}"'/g' "${FN}" > "${FN}".tmp
	mv "${FN}".tmp "${FN}"
}

#----------------------------------------------------------------
# remove_cvs_svn_tags
#
# $1 -- path where cvs/svn tags should be removed
remove_cvs_svn_tags()
{
	TARGET_DIR="${1}"

	quiet_pushd "${TARGET_DIR}"
		# remove version-control tags so that none of this can be checked in accidentally:
		echo NOTE: removing CVS tags from $PWD tree
		find . -type d -name CVS -exec rm -rf {} \; -prune

		echo NOTE: removing SVN tags from $PWD tree
		find . -type d -name .svn -exec rm -rf {} \; -prune
	quiet_popd
}

#----------------------------------------------------------------
# get_xcode_config
#
# $1 -- project directory
# $2 -- project file
# $3 -- requested configuration
get_xcode_config()
{
	PROJ_DIR=${1}
	PROJ=${2}
	REQUESTED_CONFIG=${3}
	CONFIG_ARCH=""

	quiet_pushd "${PROJ_DIR}"
		A=`xcodebuild -project "${PROJ}".xcodeproj -list`
		if [ $? != 0 ]; then exit 1; fi

		if [ ${ARCH} == "x86_64" ]; then
			AB=`echo "${A}" | grep 64`
			if [ -n "${AB}" ]; then
				CONFIG_ARCH="64"
			fi
		fi

		DEBUG_CONFIG=""
		B=`echo "${A}" | grep Development`
		if [ -n "${B}" ]; then
			DEBUG_CONFIG="Development${CONFIG_ARCH}"
		else
			B=`echo "${A}" | grep Debug`
			if [ -n "${B}" ]; then
				DEBUG_CONFIG="Debug${CONFIG_ARCH}"
			fi
		fi

		RELEASE_CONFIG=""
		B=`echo "${A}" | grep Deployment`
		if [ -n "${B}" ]; then
			RELEASE_CONFIG="Deployment${CONFIG_ARCH}"
		else
			B=`echo "${A}" | grep Release`
			if [ -n "${B}" ]; then
				RELEASE_CONFIG="Release${CONFIG_ARCH}"
			fi
		fi

		BUILD_CONFIG="${DEBUG_CONFIG}";
		if [ "${REQUESTED_CONFIG}" = "Release" ]; then
			BUILD_CONFIG="${RELEASE_CONFIG}"
		fi

		echo "${BUILD_CONFIG}"
	quiet_popd
}

#----------------------------------------------------------------
# build_xcodeproj
#
# $1 -- project directory
# $2 -- project file
# $3 -- requested configuration
# $4 -- target
# $5 -- target arch
# $6 -- Xcode SDK
build_xcodeproj()
{
	if [ -z "${ACTION}" ]; then
		ACTION=build
	fi
	PROJ_DIR=${1}
	PROJ=${2}
	REQUESTED_CONFIG=${3}
	TARGET=${4}
	TARGET_ARCH=${5}
	SDK=${6}

	if [ -n "${SDK}" ]; then
		XCODE_CMD="${XCODEBUILD} -sdk ${SDK}"
	else
		XCODE_CMD="${XCODEBUILD}"
	fi

	if [ -n "${TARGET_ARCH}" ]; then
		ARCH_CFG="ARCHS=${TARGET_ARCH}"
	else
		unset ARCH_CFG
	fi

	XCODE_CONFIG=`get_xcode_config "${PROJ_DIR}" "${PROJ}" "${REQUESTED_CONFIG}"`

	quiet_pushd "${PROJ_DIR}"
		if [ -n "${TARGET}" ]; then
			echo "${PWD};" ${XCODE_CMD} -project "${PROJ}".xcodeproj -target "${TARGET}" -configuration "${XCODE_CONFIG}" ${ARCH_CFG} "${ACTION}"
			${XCODE_CMD} -project "${PROJ}".xcodeproj -target "${TARGET}" -configuration "${XCODE_CONFIG}" ${ARCH_CFG} "${ACTION}"
		else
			echo "${PWD};" ${XCODE_CMD} -project "${PROJ}".xcodeproj -alltargets -configuration "${XCODE_CONFIG}" ${ARCH_CFG} "${ACTION}"
			${XCODE_CMD} -project "${PROJ}".xcodeproj -alltargets -configuration "${XCODE_CONFIG}" ${ARCH_CFG} "${ACTION}"
		fi

		if [ $? != 0 ]; then exit 2; fi
	quiet_popd
}


#----------------------------------------------------------------
# build_qmake
#
# $1 -- project directory
# $2 -- project file
# $3 -- requested configuration
# $4 -- target (build, install, etc)
build_qmake()
{
	PROJ_DIR="${1}"
	PROJ="${2}"
	REQUESTED_CONFIG="${3}"
	TARGET="${4}"

	quiet_pushd "${PROJ_DIR}"
		echo "${PWD};" qmake "${PROJ}".pro -spec macx-xcode
		qmake "${PROJ}".pro -spec macx-xcode
		if [ $? != 0 ]; then exit 3; fi

		XCODE_CONFIG=`get_xcode_config "${PROJ_DIR}" "${PROJ}" "${CONFIG}"`
		if [ -z "${XCODE_CONFIG}" ]; then
			echo ABORT: You must modify ${PROJ_DIR}/${PROJ}.pro for ${CONFIG} build
			exit 1
		fi

		if [ -d "${TARGET}.xcodeproj" ]; then
			build_xcodeproj "${PROJ_DIR}" "${TARGET}" "${CONFIG}" "${TARGET}"
		else
			build_xcodeproj "${PROJ_DIR}" "${PROJ}" "${CONFIG}" "${TARGET}"
		fi
	quiet_popd
}


#----------------------------------------------------------------
# build_make
#
# $1 -- project directory
# $2 -- Optional compile args
build_make()
{
	PROJ_DIR=${1}
	MAKE_ARGS=${2}

	quiet_pushd "${PROJ_DIR}"
		if [ "${ACTION}" == "clean" ]; then
			echo "${PWD};" make CONFIG="${CONFIG}" -j${NUM_THREADS} clean
			make CONFIG="${CONFIG}" -j${NUM_THREADS} clean
			# Don't exit on failure as rm fails on already clean target (no .o files)
		else
			echo "${PWD};" make CONFIG="${CONFIG}" -j${NUM_THREADS} ${MAKE_ARGS}
			make CONFIG="${CONFIG}" -j${NUM_THREADS} ${MAKE_ARGS}
			if [ $? != 0 ]; then exit 4; fi
		fi
	quiet_popd
}

#----------------------------------------------------------------
# PrepForDeployment
#
# prepare a bundle for deployment:
#
# $1 -- bundle path to prepare for deployment
# $2, $3, etc... -- Qt plugins to include
#
PrepForDeployment()
{
	BUNDLE_PATH=${1}
	shift 1

	# let Qt do it's part:
	# macdeployqt "${BUNDLE_PATH}" -verbose=3 -no-plugins

	# deploy Qt plugins manually:
	quiet_pushd "${BUNDLE_PATH}"/Contents/MacOS
		pwd

		find . -type f -print | while read i; do

			# find out which Qt was linked against and
			# copy plugins from the same version of Qt
			QT_DIR=`otool -L "${i}" | grep QtCore | grep framework | cut -f2 | cut -d' ' -f1 | rev | cut -d'/' -f6- | rev`
			if [ -z "${QT_DIR}" ]; then
				QT_DIR=`otool -L "${i}" | grep QtCore | grep dylib | cut -f2 | cut -d' ' -f1 | rev | cut -d'/' -f3- | rev`
				if [ -z "${QT_DIR}" ]; then
					continue;
				fi
			fi

			QT_DIR_IS_ABSOLUTE_PATH=`echo "${QT_DIR}" | grep -v '@loader_path' | grep -v '@executable_path'`
			if [ -z "${QT_DIR_IS_ABSOLUTE_PATH}" ]; then
				continue;
			fi

			printf '%s, QT_DIR: "%s"\n' "${i}" "${QT_DIR}"

			while [ $? = 0 ]; do
				PLUGIN="${1}"
				if [ -z "${PLUGIN}" ]; then
					break;
				fi

				echo $CP "${QT_DIR}"/plugins/"${PLUGIN}" .
				$CP "${QT_DIR}"/plugins/"${PLUGIN}" .
				shift
			done

			# get rid of debug plugins:
			find . -name '*_debug.*' -type f -print | while read i; do
				echo rm -f "${i}"
				rm -f "${i}"
			done

			unset QT_DIR
			break;
		done
		EXITCODE=$?; if [ $EXITCODE != 0 ]; then exit $EXITCODE; fi
	quiet_popd
}

#----------------------------------------------------------------
# resolve_library
#
# $1 -- library name
# $2 -- cpu arch
# $3 -- search path (optional)
#
resolve_library()
{
	NAME="${1}"
	if [ -e "${NAME}" ]; then
		# done
		echo "${NAME}"
	fi

	local NATIVE_ARCH="${2}"
	if [ -z "${NATIVE_ARCH}" ]; then
		NATIVE_ARCH=`arch`
	fi

	NAME=`basename "${NAME}"`
	SRCH_HERE="${DYLD_LIBRARY_PATH}":"/Developer/${NATIVE_ARCH}/lib"
	SRCH_HERE="${SRCH_HERE}":"/Library/Frameworks"

	if [ -n "${3}" ]; then
		SRCH_HERE="${3}:${SRCH_HERE}"
	fi

	echo "${SRCH_HERE}" | awk 'BEGIN{RS=":"}{print}' | while read i; do
		if [ ! -e "${i}" ]; then continue; fi
		find "${i}" -name "${NAME}" -print 2>/dev/null | while read j; do
			DNAME=`dirname "${j}"`
			DNAME=`(cd "${DNAME}"; pwd)`
			BNAME=`basename "${j}"`
			NAME="${DNAME}/${BNAME}"
			if [ -e "${NAME}" ]; then
				echo "${NAME}"
				return 1;
			fi
		done
		DONE=$?
	    if [ $DONE = 1 ]; then
			break;
		fi
	done
	EXITCODE=$?; if [ $EXITCODE != 0 ]; then exit $EXITCODE; fi
}

#----------------------------------------------------------------
# resolve_symlink
#
# $1 -- file path
#
resolve_symlink()
{
	SRC="${1}"
	DST=`readlink "${SRC}"`
	if [ -z "${DST}" ]; then
		echo "${SRC}"
	else
		REL_DIR=`dirname "${SRC}"`
		ABS_DIR=`(cd "${REL_DIR}"; pwd)`
		resolve_symlink "${ABS_DIR}/${DST}"
	fi
}

#----------------------------------------------------------------
# GetPathToParentDir
#
# Convert Frameworks/Some3rdParty.framework/Versions/A to ../../../..
# Convert qtplugins/imageformats to ../..
# Convert MacOS to ..
#
# $1 -- Child path (like Frameworks/Some3rdParty.framework/Versions/A)
#
GetPathToParentDir()
{
	RELATIVE_PATH="${1}"
	ABSOLUTE_PATH=`echo "${RELATIVE_PATH}" | sed 's/\.\.\///g'`
	PARENT_PATH=`echo "${ABSOLUTE_PATH}" | sed 's/[^\/]*/../g'`
	echo "${PARENT_PATH}"
}

#----------------------------------------------------------------
# DeployFileOnce
#
# $1 -- bundle contents directory path (foo.app/Contents)
# $2 -- binary to deploy (MacOS/foo, Plug-ins/bar.bundle/Contents/MacOS/bar)
# $3 -- path to the file holding a list of previously deployed files
# $4 -- library search path
#
DeployFileOnce()
{
	BASEPATH="${1}"
	FILEPATH="${2}"
	DONELIST="${3}"
	SEARCH_PATH="${4}"

	# avoid deploying the same file multiple times:
	IS_DEPLOYED=`grep "${FILEPATH}" "${DONELIST}"`
	if [ -z "${IS_DEPLOYED}" ]; then
		echo "${FILEPATH}" >> "${DONELIST}"
		echo checking "${FILEPATH}"
		(DeployFile "${BASEPATH}" "${FILEPATH}" "${DONELIST}" "${SEARCH_PATH}")
		local EXITCODE=$?
		echo
		if [ $EXITCODE != 0 ]; then
			exit $EXITCODE
		fi
	fi
}

#----------------------------------------------------------------
# GetRunpath
#
# $1 -- path to an executable or a shared library
#
GetRunpath()
{
	otool -l "${1}" | \
	while read i; do
		if [ "${i}" = 'cmd LC_RPATH' ]; then
			read cmdsize
			read lc_rpath
			lc_rpath=$(echo "${lc_rpath}" | cut -d' ' -f2)
			printf "${lc_rpath}:"
		fi
	done
}

#----------------------------------------------------------------
# PrependPath
#
# $1 -- new path entries
# $2 -- existing path
#
PrependPath()
{
	echo "${1}" | \
		awk 'BEGIN{RS=":"}{print}' | \
		while read i; do \
			if [[ "${2}" == *"${i}"* ]]; then
				continue
			else
				printf "${i}:"
			fi
		done
	printf "${2}\n"
}

#----------------------------------------------------------------
# DeployFile
#
# $1 -- bundle contents directory path (foo.app/Contents)
# $2 -- binary to deploy (MacOS/foo, Plug-ins/bar.bundle/Contents/MacOS/bar)
# $3 -- path to the file holding a list of previously deployed files
# $4 -- library search path
#
DeployFile()
{
	BASEPATH="${1}"
	FILEPATH="${2}"
	DONELIST="${3}"
	SEARCH_PATH="${4}"

#	echo
#	echo BASEPATH: ${BASEPATH}
#	echo FILEPATH: ${FILEPATH}

	if [ ! -e "${FILEPATH}" ]; then
		printf "MISSING: %s\n" "${BASEPATH}/${FILEPATH}"
		exit 11
	fi

	SEARCH_RPATH=`GetRunpath "${FILEPATH}"`
	SEARCH_PATH=`PrependPath "${SEARCH_RPATH}" "${SEARCH_PATH}"`

	FILE=`basename "${FILEPATH}"`
	FDIR=`dirname "${FILEPATH}"`
	quiet_pushd "${FDIR}"

#	echo FILE: ${FILE}
#	echo FDIR: ${FDIR}

	IS_PLUGIN=`echo "${FILEPATH}" | grep -i 'plug'`
	if [ -n "${IS_PLUGIN}" ]; then
		printf "PLUGIN\n"
	fi

	otool -L "${FILE}" | grep -v "${FILE}" | rev | cut -d'(' -f2 | rev | while read i; do
		NEEDS="${i}"
		IS_USR_LIB=`echo "${NEEDS}" | grep '/usr/lib/'`
		IS_SYS_LIB=`echo "${NEEDS}" | grep '/System/Library/'`
		IS_CUDA_DRIVER=`echo "${NEEDS}" | grep '/usr/local/cuda/lib/libcuda.dylib'`
		if [ -n "${IS_USR_LIB}" -o \
		     -n "${IS_SYS_LIB}" -o \
		     -n "${IS_CUDA_DRIVER}" ]; then
#			echo skipping system library "${NEEDS}"
			continue
		fi

		FILE_ARCH=`lipo -info "${FILE}" | rev | cut -d' ' -f1 | rev`
		IS_FRAMEWORK=`echo "${NEEDS}" | grep '\.framework/'`
		IS_DEBUG=`echo "${NEEDS}" | grep _debug`
		AT_RPATH=`echo "${NEEDS}" | grep '@rpath/'`
		AT_LOAD_PATH=`echo "${NEEDS}" | grep '@loader_path/'`
		AT_EXEC_PATH=`echo "${NEEDS}" | grep '@executable_path/'`

		if [ -z "${IS_FRAMEWORK}" -a \
			 -z "${AT_LOAD_PATH}" -a \
			 -z "${AT_EXEC_PATH}" -a \
			 -z "${AT_RPATH}" ]; then
			ORIGIN_DIR=$(dirname "${NEEDS}")
			SEARCH_PATH=`PrependPath "${ORIGIN_DIR}" "${SEARCH_PATH}"`
		fi

		printf "%40s : " "${FILE}"

		if [ -n "${IS_FRAMEWORK}" ]; then
			printf "framework, "
		else
			printf "dylib, "
		fi

#		if [ -n "${IS_DEBUG}" ]; then
#			printf "debug, "
#			IS_QTLIB=`echo "${NEEDS}" | grep Qt`
#			if [ -n "${IS_QTLIB}" ]; then
#				NO_DEBUG=`echo "${NEEDS}" | sed 's/_debug//'`
#				printf "changing %s to %s, " "${NEEDS}" "${NO_DEBUG}"
#				NEEDS="${NO_DEBUG}"
#			fi
#		fi

		if [ -n "${AT_LOAD_PATH}" ]; then
			printf "@load, "
		elif [ -n "${AT_EXEC_PATH}" ]; then
			printf "@exec, "
		elif [ -n "${AT_RPATH}" ]; then
			printf "@rpath, "
		fi
		printf "%s\n" `basename "${NEEDS}"`

		if [ -n "${AT_LOAD_PATH}" -o -n "${AT_RPATH}" ]; then
			REF=`echo "${NEEDS}" | cut -d/ -f2-`
			if [ -e "${REF}" ]; then
				DeployFileOnce "${BASEPATH}" "${REF}" "${DONELIST}" "${SEARCH_PATH}"
				if [ $? != 0 ]; then exit 11; fi
			else
				FOUND=`resolve_library "${REF}" "${FILE_ARCH}" "${SEARCH_PATH}"`
				if [ ! -e "${FOUND}" ]; then exit 21; fi

				NEEDS="${FOUND}"
			fi
		fi

		if [ -n "${AT_EXEC_PATH}" ]; then
#			echo CHANGING EXEC PATH to LOAD PATH:
			REF=`echo "${NEEDS}" | cut -d/ -f2- | cut -c4-`
			NEEDS="${BASEPATH}"/"${REF}"
		fi

		printf "%s (in %s) NEEDS %s\n" "${FILE}" "${FDIR}" "${NEEDS}"

		if [ -n "${IS_FRAMEWORK}" ]; then
			DST="Frameworks"
		else
			DST="Auxiliaries"
		fi

		HAS_VERSION=`echo "${NEEDS}" | grep /Versions/`
		if [ -n "${HAS_VERSION}" ]; then
			FN_DST=`echo "${NEEDS}" | rev | cut -d/ -f-4 | rev`
		elif [ -n "${IS_FRAMEWORK}" ]; then
			FN_DST=`echo "${NEEDS}" | rev | cut -d/ -f-2 | rev`
		else
			FN_DST=`echo "${NEEDS}" | rev | cut -d/ -f-1 | rev`
		fi
#		echo FN_DST: ${FN_DST}

		DST_DIR=`dirname "${DST}/${FN_DST}"`
#		echo DST_DIR: ${DST_DIR}

		if [ ! -e "${BASEPATH}/${DST_DIR}" ]; then
			echo mkdir -p "${BASEPATH}/${DST_DIR}"
			mkdir -p "${BASEPATH}/${DST_DIR}"
		fi

		OFFSET=`GetPathToParentDir "${FDIR}"`
#		echo FDIR: ${FDIR} -- OFFSET: ${OFFSET}

		DST_NAME="@loader_path/${OFFSET}/${DST}/${FN_DST}"

#		echo CHECK IF EXISTS: "${BASEPATH}/${DST}/${FN_DST}"
		if [ ! -e "${BASEPATH}/${DST}/${FN_DST}" ]; then
			SRC="${NEEDS}"
			if [ ! -e "${SRC}" ]; then
#				echo resolve_library "${SRC}" "${FILE_ARCH}"
				SRC=`resolve_library "${SRC}" "${FILE_ARCH}"`
#				echo resolve_library returned \""${SRC}"\"
				if [ -e "${SRC}" ]; then
					echo "RESOLVED ${NEEDS} --- ${SRC}"
				fi
			fi

			SRC=`resolve_symlink "${SRC}"`
#			echo resolve_symlink returned \""${SRC}"\"

			if [ ! -e "${SRC}" ]; then
				echo "MISSING: ${NEEDS}"
				exit 11
			fi

			echo ${CP} "${SRC}" "${BASEPATH}/${DST}/${FN_DST}"
			${CP} "${SRC}" "${BASEPATH}/${DST}/${FN_DST}"

			# copy resources bundled together with the framework:
			SRC_BASE=`dirname "${SRC}"`
			if [ -e "${SRC_BASE}/Resources" ]; then
				DST_BASE=`dirname "${BASEPATH}/${DST}/${FN_DST}"`
				echo mkdir -p "${DST_BASE}/Resources";
				mkdir -p "${DST_BASE}/Resources";
				quiet_pushd "${DST_BASE}/Resources"
					pwd
					echo COPYING "${SRC_BASE}/Resources"
					(cd "${SRC_BASE}/Resources"; tar c .) | tar xv
				quiet_popd
			fi

			if [ -n "${HAS_VERSION}" ]; then
				VERTMP=`dirname "${BASEPATH}/${DST}/${FN_DST}"`
				VERSION=`basename "${VERTMP}"`

				VERTMP=`dirname "${VERTMP}"`
				VERSIONS="${VERTMP}"

				quiet_pushd "${VERSIONS}"
					pwd
					if [ ! -e Current ]; then
						echo ln -s "${VERSION}" Current
						ln -s "${VERSION}" Current
					fi
				quiet_popd

				VERTMP=`dirname "${VERTMP}"`
				VERSIONS="${VERTMP}"
				quiet_pushd "${VERSIONS}"
					pwd

					if [ -e "Versions/${VERSION}/Resources" ]; then
						echo ln -s "Versions/${VERSION}/Resources" .
						ln -s "Versions/${VERSION}/Resources" .
					fi

					DSTTMP=`basename "${FN_DST}"`
					echo ln -s "Versions/${VERSION}/${DSTTMP}" .
					ln -s "Versions/${VERSION}/${DSTTMP}" .
				quiet_popd
			fi
		fi

		quiet_pushd "${BASEPATH}"
			DeployFileOnce "${BASEPATH}" "${DST}/${FN_DST}" "${DONELIST}" "${SEARCH_PATH}"
			if [ $? != 0 ]; then exit 11; fi
		quiet_popd

		echo "${INSTALL_NAME_TOOL}" -change "${i}" "${DST_NAME}" "${FILE}"
		"${INSTALL_NAME_TOOL}" -change "${i}" "${DST_NAME}" "${FILE}"
		if [ $? != 0 ]; then
			# install_name_tool failure may be fixed by using
			# the -headerpad_max_install_names linker flag
			exit 13;
		fi
	done
	EXITCODE=$?; if [ $EXITCODE != 0 ]; then exit $EXITCODE; fi

	quiet_popd
}

#----------------------------------------------------------------
# DeployAppBundle
#
# $1 -- Bundle path (/path/to/foo.app)
#
DeployAppBundle()
{
	BUNDLE_PATH="${1}"

	if [ -z "${TMPDIR}" ]; then
		TMPDIR=/tmp
	fi

	# create a temp file to keep the list of deployed files:
	DONELIST=`mktemp -t DeployOSX.sh` || exit 12

	quiet_pushd "${BUNDLE_PATH}"/Contents
	BASE=`pwd`
	find MacOS -type f -print | while read i; do
		DeployFileOnce "${BASE}" "${i}" "${DONELIST}"
		EXITCODE=$?
		if [ $EXITCODE != 0 ]; then
			rm -f "${DONELIST}"
			exit $EXITCODE;
		fi
	done
	EXITCODE=$?; if [ $EXITCODE != 0 ]; then exit $EXITCODE; fi

	find Plug-ins -type f -print 2>/dev/null | grep MacOS | while read i; do
		DeployFileOnce "${BASE}" "${i}" "${DONELIST}"
		EXITCODE=$?
		if [ $EXITCODE != 0 ]; then
			rm -f "${DONELIST}"
			exit $EXITCODE;
		fi
	done
	EXITCODE=$?; if [ $EXITCODE != 0 ]; then exit $EXITCODE; fi

	find Plug-ins -type f -path '*/Versions/*/*' -print 2>/dev/null | grep -v Resources | while read i; do
		DeployFileOnce "${BASE}" "${i}" "${DONELIST}"
		EXITCODE=$?
		if [ $EXITCODE != 0 ]; then
			rm -f "${DONELIST}"
			exit $EXITCODE;
		fi
	done
	EXITCODE=$?; if [ $EXITCODE != 0 ]; then exit $EXITCODE; fi

	find Frameworks -type f -path '*/Versions/*/*' -print 2>/dev/null | grep -v Resources | while read i; do
		DeployFileOnce "${BASE}" "${i}" "${DONELIST}"
		EXITCODE=$?
		if [ $EXITCODE != 0 ]; then
			rm -f "${DONELIST}"
			exit $EXITCODE;
		fi
	done
	EXITCODE=$?; if [ $EXITCODE != 0 ]; then exit $EXITCODE; fi

	find MacOS -type d -print | while read i; do
		find "${i}" -type f -print | while read j; do
			DeployFileOnce "${BASE}" "${j}" "${DONELIST}"
			EXITCODE=$?
			if [ $EXITCODE != 0 ]; then
				rm -f "${DONELIST}"
				exit $EXITCODE;
			fi
		done
		EXITCODE=$?; if [ $EXITCODE != 0 ]; then exit $EXITCODE; fi
	done
	EXITCODE=$?; if [ $EXITCODE != 0 ]; then exit $EXITCODE; fi
	quiet_popd

	rm -f "${DONELIST}"
}

#---------------------------------------------------------------------
echo NEXT: copy required frameworks into the app bundle
PrepForDeployment "${BUNDLE_PATH}" accessible codecs graphicssystems imageformats

#---------------------------------------------------------------------
echo NEXT: deploy the app bundle
DeployAppBundle "${BUNDLE_PATH}"
