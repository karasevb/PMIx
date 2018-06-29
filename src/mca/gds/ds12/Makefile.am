# -*- makefile -*-
#
# Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2012      Los Alamos National Security, Inc.  All rights reserved.
# Copyright (c) 2013-2017 Intel, Inc. All rights reserved.
# Copyright (c) 2017      Research Organization for Information Science
#                         and Technology (RIST). All rights reserved.
# Copyright (c) 2017      Mellanox Technologies, Inc.
#                         All rights reserved.
# $COPYRIGHT$
#
# Additional copyrights may follow
#
# $HEADER$
#

headers = \
        ../ds_common/gds_dstore_common.h \
        gds_dstore_v12.h \
        ../ds_common/dstore_seg.h
#        ../ds_common/session.h
#       nspace_info.h \
#       session.h
#        pthread_lock_v12.h

sources = \
        ../ds_common/gds_dstore_common.c \
        gds_dstore_v12.c \
        gds_dstore_v12_component.c \
        ../ds_common/dstore_seg.c
#        ../ds_common/session.c
#        nspace_info.c \
#        session.c
#        pthread_lock_v12.c

# Make the output library in this directory, and name it either
# mca_<type>_<name>.la (for DSO builds) or libmca_<type>_<name>.la
# (for static builds).

if MCA_BUILD_pmix_gds_ds12_DSO
lib =
lib_sources =
component = mca_gds_ds12.la
component_sources = $(headers) $(sources)
else
lib = libmca_gds_ds12.la
lib_sources = $(headers) $(sources)
component =
component_sources =
endif

mcacomponentdir = $(pmixlibdir)
mcacomponent_LTLIBRARIES = $(component)
mca_gds_ds12_la_SOURCES = $(component_sources)
mca_gds_ds12_la_LDFLAGS = -module -avoid-version

noinst_LTLIBRARIES = $(lib)
libmca_gds_ds12_la_SOURCES = $(lib_sources)
libmca_gds_ds12_la_LDFLAGS = -module -avoid-version
