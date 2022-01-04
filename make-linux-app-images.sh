PROJ_WC_REVISION=`grep -RI git.replay.20220104.001326.a9c2fa4b ./yae/api/yae_version.h | cut -d'"' -f2`

rm -rf AppDir* *.AppImage
make install DESTDIR=AppDir

rm -f AppDir.tar.bz2
tar cfj AppDir.tar.bz2 AppDir
rm -rf AppDir

deploy_appdir()
{
    APP="${1}"

    rm -rf "AppDir.${APP}"/home
    rm -f "AppDir.${APP}"/usr/bin/*.a

    for i in "AppDir.${APP}"/usr/bin/*; do
	Z=`echo "${i}" | grep "${APP}"\$`
	
	if [ -z "${Z}" ]; then
	    rm -f "${i}"
	fi
    done

    for i in "AppDir.${APP}"/usr/share/applications/*.desktop; do
	Z=`echo "${i}" | grep "${APP}.desktop"`
	if [ -z "${Z}" ]; then
	    rm -f "${i}"
	fi
    done

    for i in "AppDir.${APP}"/usr/share/icons/*.png; do
	Z=`echo "${i}" | grep "${APP}.png"`
	if [ -z "${Z}" ]; then
	    rm -f "${i}"
	fi
    done

    cat "AppDir.${APP}"/usr/share/applications/"${APP}".desktop | \
	sed 's/\/usr\/share\/icons\/\(.*\)\.png/\1/g' | \
	sed 's/\/usr\/bin\///g' \
	> /tmp/"${APP}".desktop

    cat /tmp/"${APP}".desktop > "AppDir.${APP}"/usr/share/applications/"${APP}".desktop

    tree "AppDir.${APP}"

    #FIXME:
    #exit 0

    mv -f "AppDir.${APP}"/usr/share/icons/"${APP}".png /tmp/"${APP}".png

    LD_LIBRARY_PATH="${LD_LIBRARY_PATH}:${HOME}"/x86_64/lib \
	linuxdeploy-x86_64.AppImage \
	--appdir "AppDir.${APP}" \
	--icon-file /tmp/"${APP}".png \
	--output appimage || exit 1

    tree "AppDir.${APP}"/usr/lib

    mkdir -p .dist
    FN_OUT="${APP}-${PROJ_WC_REVISION}-x86_64.AppImage"
    rm -f .dist/"${FN_OUT}"
    mv *.AppImage .dist/"${FN_OUT}"
}


for APP in yaetv apprenticevideo-classic apprenticevideo aeyaeremux; do
    tar xfj AppDir.tar.bz2
    rm -rf AppDir."${APP}"
    mv AppDir AppDir."${APP}"
    deploy_appdir "${APP}"
done
