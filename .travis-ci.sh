#!/bin/bash
# Based on a test script from avsm/ocaml repo https://github.com/avsm/ocaml

CHROOT_DIR=/tmp/mips-chroot
MIRROR=http://ftp.us.debian.org/debian/
VERSION=stretch
CHROOT_ARCH=mips

# Debian package dependencies for the host
HOST_DEPENDENCIES="debootstrap qemu-user-static binfmt-support sbuild schroot"

# Debian package dependencies for the chrooted environment
GUEST_DEPENDENCIES="build-essential libssl-dev cmake g++"

# Command used to run the tests
TEST_COMMAND="make test"

function setup_mips_chroot {
    # Host dependencies
    sudo apt-get update
    sudo apt-get upgrade -qq -y
    sudo apt-get install -qq -y ${HOST_DEPENDENCIES}

    # Create chrooted environment
    sudo mkdir ${CHROOT_DIR}
    sudo debootstrap --foreign --no-check-gpg --include=fakeroot,build-essential \
        --arch=${CHROOT_ARCH} ${VERSION} ${CHROOT_DIR} ${MIRROR}
    sudo cp /usr/bin/qemu-mips-static ${CHROOT_DIR}/usr/bin/
    sudo chroot ${CHROOT_DIR} ./debootstrap/debootstrap --second-stage
    sudo sbuild-createchroot --arch=${CHROOT_ARCH} --foreign --setup-only \
        ${VERSION} ${CHROOT_DIR} ${MIRROR}

    # Create file with environment variables which will be used inside chrooted
    # environment
    echo "export ARCH=${ARCH}" > envvars.sh
    echo "export TRAVIS_BUILD_DIR=${TRAVIS_BUILD_DIR}" >> envvars.sh
    chmod a+x envvars.sh

    # Install dependencies inside chroot
    sudo chroot ${CHROOT_DIR} apt-get update
    sudo chroot ${CHROOT_DIR} apt-get --allow-unauthenticated install \
        -qq -y ${GUEST_DEPENDENCIES}

    # Create build dir and copy travis build files to our chroot environment
    sudo mkdir -p ${CHROOT_DIR}/${TRAVIS_BUILD_DIR}
    sudo rsync -av ${TRAVIS_BUILD_DIR}/ ${CHROOT_DIR}/${TRAVIS_BUILD_DIR}/

    # Indicate chroot environment has been set up
    sudo touch ${CHROOT_DIR}/.chroot_is_done

    # Call ourselves again which will cause tests to run
    sudo chroot ${CHROOT_DIR} bash -c "cd ${TRAVIS_BUILD_DIR} && bash -ex .travis-ci.sh"
}

if [ -e "/.chroot_is_done" ]; then
  # We are inside MIPS chroot
  echo "Running inside chrooted environment"

  . ./envvars.sh
else
  if [ "${ARCH}" = "mips" ]; then
    # MIPS test run, need to set up chrooted environment first
    echo "Setting up chrooted MIPS environment"
    setup_mips_chroot

    # Copy the artifact out of the chroot
    cp ${CHROOT_DIR}/${TRAVIS_BUILD_DIR}/${OUTPUT_BINARY} ${TRAVIS_BUILD_DIR}/

    # Don't run the build twice
    exit
  fi
fi

echo "Performing build"
echo "Environment: $(uname -a)"

mkdir ${TRAVIS_BUILD_DIR}/build
cd ${TRAVIS_BUILD_DIR}/build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j4

echo "Running tests"
make test && cp dote ../${OUTPUT_BINARY}
