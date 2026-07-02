# librttopo dependencies

|project|license [^_l]|description [dependencies]|version|source|diff [^_d]|
|-------|-------------|--------------------------|-------|------|----------|
|<a id='librttopo' />[librttopo](https://git.osgeo.org/gitea/rttopo/librttopo)|[GPL-2.0-or-later](https://github.com/CGX-GROUP/librttopo/blob/master/COPYING 'GNU General Public License v2.0 or later')|RT Topology Library exposes an API to create and manage standard topologies using user-provided data stores [deps: _geos_]| |[upstream](https://github.com/CGX-GROUP/librttopo 'github.com/CGX-GROUP/librttopo')|  [auto]|
|<a id='geos' />[geos](https://libgeos.org)|[LGPL-2.1-only](https://trac.osgeo.org/geos/ 'LGPL version 2.1')|C/C++ library for computational geometry with a focus on algorithms used in geographic information systems (GIS) software|[xpv3.14.1.1](https://github.com/externpro/geos/releases/tag/xpv3.14.1.1 'release')|[repo](https://github.com/externpro/geos 'github.com/externpro/geos') [upstream](https://github.com/libgeos/geos 'github.com/libgeos/geos')|[diff](https://github.com/externpro/geos/compare/3.14.1...xpv3.14.1.1 'github.com/externpro/geos/compare/3.14.1...xpv3.14.1.1') [patch]|

![deps](xprodeps.svg 'dependencies')

Dependency version check: all 1 parent-manifest versions match pinned versions.

|diff  |description|
|------|-----------|
|patch |diff modifies/patches existing cmake|
|intro |diff introduces cmake|
|auto  |diff adds cmake to replace autotools/configure/make|
|native|diff adds cmake but uses existing build system|
|bin   |diff adds cmake to repackage binaries built elsewhere|
|fetch |diff adds cmake and utilizes FetchContent|

[^_l]: see [SPDX License List](https://spdx.org/licenses/ '') for a list of commonly found licenses
[^_d]: see table above with description of diff
