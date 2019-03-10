#!/usr/bin/env bash

export LD_LIBRARY_PATH=$HOME/local-installs/lib
input_md=$1

if [ ! -f "$input_md" ] ; then
  echo "error: no input file: '$input_md'"
  exit 1
fi

TARGET_LIBYANG_DIR=$PWD/lib/libyang_install
if [ ! -f lib/yang-json-check ]; then
  set -x
  echo "building yang-json-check..."
  pushd lib

  if [ ! -d ${TARGET_LIBYANG_DIR} ]; then
    echo "building libyang"
    git clone https://github.com/CESNET/libyang
    mkdir libyang/build
    pushd libyang/build
    cmake -DCMAKE_INSTALL_PREFIX=${TARGET_LIBYANG_DIR} ..
    make && make install
    popd
  fi
  gcc -o yang-json-check -I ${TARGET_LIBYANG_DIR}/include yang-json-check.c -L ${TARGET_LIBYANG_DIR}/lib -lyang
  popd
  set +x
fi
export LD_LIBRARY_PATH=${TARGET_LIBYANG_DIR}/lib

modules=$(grep YANG-MODULE $input_md | awk '{print $2;}')

# check module and data examples
for input_module in ${modules}; do
  echo "input: $input_module"

  if [ ! -f "$input_module" ] ; then
    echo "error: no input file: '$input_module' from YANG-MODULE in $input_md"
    exit 1
  fi

  if ! pyang --verbose --ietf --lint --max-line-length 72 --path modules/:. ${input_module} ; then
    echo "checking for missing modules, downloading into modules/..."
    mkdir -p modules
    before=$(ls modules/ | wc -l)
    pyang --path modules/ ${input_module} 2>&1 | \
       grep "error: module" | \
       sed -e 's/.*: error: module "\([^"]*\)" not found in search path.*/\1/' | \
       xargs -I {} rsync -cvz rsync.iana.org::assignments/yang-parameters/{}*.yang modules/
    after=$(ls modules/ | wc -l)
    if [ "$before" = "$after" ]; then
      exit 1
    fi
    echo "fetched $(($after - $before)) modules, re-running checks:"
    pyang --verbose --ietf --lint --max-line-length 72 --path modules/:. ${input_module} || exit 1
  fi

  examples=$(grep -E "YANG-DATA +${input_module}" $input_md | awk '{print $3;}')
  for ex in ${examples}; do
    echo "example: $ex"
    if [ ! -f "$ex" ] ; then
      echo "error: no input file: '$ex' from YANG-DATA in $input_md"
      exit 1
    fi
    if ! lib/yang-json-check $input_module $ex; then
      echo "failed data parse: $ex"
      exit 1
    fi
  done
done

lib/yang-inject.py $input_md

