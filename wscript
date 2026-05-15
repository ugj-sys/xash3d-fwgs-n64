#! /usr/bin/env python
# encoding: utf-8
# a1batross, mittorn, 2018

from __future__ import print_function
from waflib import Logs, Context, Configure
import sys
import os

VERSION = '0.99'
APPNAME = 'xash3d-fwgs'
top = '.'

Context.Context.line_just = 55 # should fit for everything on 80x26

class Subproject:
	name      = ''
	dedicated = True  # if true will be ignored when building dedicated server
	singlebin = False # if true will be ignored when singlebinary is set
	ignore    = False # if true will be ignored, set by user request
	mandatory  = False
	onlypsp = False

	def __init__(self, name, dedicated=True, singlebin=False, mandatory = False, onlypsp = False):
		self.name = name
		self.dedicated = dedicated
		self.singlebin = singlebin
		self.mandatory = mandatory
		self.onlypsp = onlypsp

	def is_enabled(self, ctx):
		if not self.mandatory:
			if self.name in ctx.env.IGNORE_PROJECTS:
				self.ignore = True

		if self.ignore:
			return False

		if ctx.env.SINGLE_BINARY and self.singlebin:
			return False





		if ctx.env.DEDICATED and self.dedicated:
			return False

		return True

SUBDIRS = [
	Subproject('public',      dedicated=False, mandatory = True),
	Subproject('game_launch', singlebin=True),
	Subproject('ref_soft'),
	Subproject('engine', dedicated=False),
]

def subdirs():
	return map(lambda x: x.name, SUBDIRS)

def options(opt):
	grp = opt.add_option_group('Common options')

	grp.add_option('-T', '--build-type', action='store', dest='BUILD_TYPE', default = None,
		help = 'build type: debug, release or none(custom flags)')

	grp.add_option('-d', '--dedicated', action = 'store_true', dest = 'DEDICATED', default = False,
		help = 'build Xash Dedicated Server [default: %default]')

	grp.add_option('--single-binary', action = 'store_true', dest = 'SINGLE_BINARY', default = False,
		help = 'build single "xash" binary (always enabled for dedicated) [default: %default]')

	grp.add_option('--enable-bsp2', action = 'store_true', dest = 'SUPPORT_BSP2_FORMAT', default = False,
		help = 'build engine and renderers with BSP2 map support(recommended for Quake, breaks compatibility!) [default: %default]')

	grp.add_option('--enable-lto', action = 'store_true', dest = 'LTO', default = False,
		help = 'enable Link Time Optimization if possible [default: %default]')

	grp.add_option('--enable-poly-opt', action = 'store_true', dest = 'POLLY', default = False,
		help = 'enable polyhedral optimization if possible [default: %default]')

	grp.add_option('--low-memory-mode', action = 'store', dest = 'LOW_MEMORY', default = 0, type = 'int',
		help = 'enable low memory mode (only for devices have <128 ram)')

	grp.add_option('--enable-bsp2', action = 'store_true', dest = 'SUPPORT_BSP2_FORMAT', default = False,
		help = 'build engine and renderers with BSP2 map support(recommended for Quake, breaks compatibility!) [default: %default]')

	grp.add_option('--enable-lto', action = 'store_true', dest = 'LTO', default = False,
		help = 'enable Link Time Optimization if possible [default: %default]')

	grp.add_option('--enable-poly-opt', action = 'store_true', dest = 'POLLY', default = False,
		help = 'enable polyhedral optimization if possible [default: %default]')

	grp.add_option('--low-memory-mode', action = 'store', dest = 'LOW_MEMORY', default = 0, type = 'int',
		help = 'enable low memory mode (only for devices have <128 ram)')

	grp.add_option('--ext-opt-mode', action = 'store', dest = 'EXT_OPT', default = 0, type = 'int',
		help = 'enable extended optimization mode (only for weak devices)')

	grp.add_option('--enable-magx', action = 'store_true', dest = 'MAGX', default = False,
		help = 'enable targeting for MotoMAGX phones [default: %default]')

	grp.add_option('--enable-profiling', action = 'store_true', dest = 'PROFILING', default = False,
		help = 'enable building with GNU profiling tools')

	grp.add_option('--ignore-projects', action = 'store', dest = 'IGNORE_PROJECTS', default = None,
		help = 'disable selected projects from build [default: %default]')

	opt.load('subproject')

	for i in SUBDIRS:
		if not i.mandatory and not opt.path.find_node(i.name+'/wscript'):
			i.ignore = True
			continue

		opt.add_subproject(i.name)


	opt.load('xshlib xcompile compiler_cxx compiler_c sdl2 clang_compilation_database strip_on_install waf_unit_test')
	# Removed Windows platform specific option loading to focus on N64 MIPS target
	opt.load('reconfigure')

def configure(conf):
	enforce_pic = True # modern defaults
	valid_build_types = ['fastnative', 'fast', 'release', 'debug', 'nooptimize', 'sanitize', 'none']
	conf.load('fwgslib reconfigure')

	if conf.options.IGNORE_PROJECTS:
		conf.env.IGNORE_PROJECTS = conf.options.IGNORE_PROJECTS.split(',')

	conf.start_msg('Build type')
	if conf.options.BUILD_TYPE == None:
		conf.end_msg('not set', color='RED')
		conf.fatal('Please set a build type, for example "-T release"')
	elif not conf.options.BUILD_TYPE in valid_build_types:
		conf.end_msg(conf.options.BUILD_TYPE, color='RED')
		conf.fatal('Invalid build type. Valid are: %s' % ', '.join(valid_build_types))
	conf.end_msg(conf.options.BUILD_TYPE)

	# -march=native should not be used
	if conf.options.BUILD_TYPE.startswith('fast'):
		Logs.warn('WARNING: \'%s\' build type should not be used in release builds', conf.options.BUILD_TYPE)

	# Windows/MSVC platform setup removed to focus on N64/MIPS compilation path.

	try:
		conf.env.CC_VERSION[0]
	except IndexError:
		conf.env.CC_VERSION = (0,)

	# modify options dictionary early

	# Since we are targeting N64/MIPS exclusively, we set the environment to reflect this primary target path.
	# This simplifies configuration and removes dependency on other operating systems (Windows, Android, PSP).

	# Set defaults appropriate for console/embedded system development (N64 mimic)
	enforce_pic = False # PIC often breaks static linking in embedded environments
	conf.options.NO_VGUI = True
	conf.options.SINGLE_BINARY = True
	conf.options.NO_ASYNC_RESOLVE = True
	conf.options.LOW_MEMORY = 2
	conf.options.EXT_OPT = 2
	# Conf.options.GL is usually False for N64 emulation/embedded target
	conf.options.GL = False
	conf.options.STATIC = True

	# Define static linking list specifically for ref_soft, as it contains core logic
	static_list = ['ref_soft'] if conf.options.N64_OPTS else ['menu', 'ref_soft']
	ignore_list = ['ref_gl', 'stub/server', 'stub/client', 'mainui'] # Exclude modern platform refs
	conf.options.STATIC_LINKING = ','.join(static_list)
	conf.env.IGNORE_PROJECTS = ignore_list
	conf.load('xshlib xcompile gas compiler_c compiler_cxx')

	# --- General Platform Checks (Simplified for Unix-like embedded build system) ---
	try:
		conf.env.CC_VERSION[0] # Ensure CC_VERSION is populated
	except IndexError:
		conf.env.CC_VERSION = (0,)

	if conf.options.BUILD_TYPE == None:
		conf.end_msg('not set', color='RED')
		conf.fatal('Please set a build type, for example "-T release"')
	elif not conf.options.BUILD_TYPE in valid_build_types:
		conf.end_msg(conf.options.BUILD_TYPE, color='RED')
		conf.fatal('Invalid build type. Valid are: %s' % ', '.join(valid_build_types))

	# Minimal libraries check for Unix-like systems (needed for general linking logic)
	conf.check_cc(lib='dl', mandatory=False)

	if conf.env.DEST_OS == 'n64':
		conf.env.LIB_M = ['m']

	if not conf.env.LIB_M: # HACK: already added in xcompile!
		conf.check_cc(lib='m')

	# indicate if we are packaging for embedded/Unix-like systems
	conf.env.LIBDIR = conf.env.BINDIR = '${PREFIX}/lib/xash3d'

	conf.define('XASH_BUILD_COMMIT', conf.env.GIT_VERSION if conf.env.GIT_VERSION else 'notset')

	if conf.options.LOW_MEMORY:
		conf.define('XASH_LOW_MEMORY', conf.options.LOW_MEMORY)

	if conf.options.EXT_OPT:
		conf.define('XASH_EXT_OPT', conf.options.EXT_OPT)

	if conf.options.PROFILING:
		conf.define('XASH_PROFILING', 1)

	conf.env.SINGLE_BINARY = conf.options.SINGLE_BINARY
	conf.define('SINGLE_BINARY', 1 if conf.options.SINGLE_BINARY else 0)
	conf.define('STDINT_H', 'stdint.h')
	conf.define('HAVE_TGMATH_H', 1)

	for i in SUBDIRS:
		if not i.is_enabled(conf):
			continue

		conf.add_subproject(i.name)

def build(bld):
	bld.load('xshlib xcompile gas compiler_c compiler_cxx')
	for i in SUBDIRS:
		if not i.is_enabled(bld):
			continue

		bld.add_subproject(i.name)
