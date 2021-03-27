#! /bin/sh

version=$(head -5 meson.build | grep version | sed -e "s/[^']*'//" -e "s/'.*$//")
release_build_dir="release_build"
branch=$(git branch --show-current)

if [ -d ${release_build_dir} ]; then
  echo "Please remove ./${release_build_dir} first"
  exit 1
fi

# make sure included subprojects are current
meson subprojects update gi-docgen

# make the release tarball
meson setup -Dgtk_doc=true --force-fallback-for gi-docgen ${release_build_dir} || exit
meson compile -C${release_build_dir} || exit
meson dist -C${release_build_dir} --include-subprojects || exit


echo -e "\n\nPango ${version} release on branch ${branch} in ./${release_build_dir}/:\n"

ls -l --sort=time -r "${release_build_dir}/meson-dist"

echo -e "\nPlease sanity-check these tarballs before uploading them."
