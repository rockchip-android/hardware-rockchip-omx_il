#!/bin/bash
local_path=$1
# 生成 svn 状态信息
cd $local_path
git_info_txt=`git log \-1`; ret_info=$?
if [ $ret_info -eq 0 ]
then {
    git_commit=`git log \-1 | grep commit | head -n 1`;
} fi
sf_author=$LOGNAME
sf_date=`date -R`
# 处理生成的状态信息
echo "#define OMX_COMPILE_INFO      \"author:  $sf_author\n time: $sf_date git $git_commit \"" > ./include/rockchip/git_info.h
