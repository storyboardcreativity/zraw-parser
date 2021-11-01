# zraw-parser

This is a free draft ZRAW decoder. This tool allows to extract ZRAW video files to DNG sequences or HEVC bitstreams (according to camera firmware version).

### What ZRAW versions are compatible with this software?

For versions before 0.94 you can get pure Bayer CFA, for later versions - you get real HEVC bitstream extracted from ".ZRAW" file without any reencoding process.

### How to build?

##### For Ubuntu:

0. Install all Nana C++ GUI dependencies

1. Clone Nana C++ GUI library
Example for v1.7.4: `git clone --depth 1 --branch v1.7.4 https://github.com/cnjinhao/nana`

2. Build Nana C++ GUI library
`cd nana/build/makefile/`
`make`

3. Copy `nana/build/bin/libnana.a` to `<ZRAW-Parser root folder>/lib/`

4. Copy `nana/include/nana/*` to `<ZRAW-Parser root folder>/include/nana/`

5. Type `make` and have fun!

### License

Copyright 2021 storyboardcreativity

This software is created solely on a non-commercial basis to ensure compatibility with ZRAW file formats and is licensed under the GNU General Public License version 2 (the "GPL License").

You may obtain a copy of the GPL License in the LICENSE file, or at:

http://www.gnu.org/licenses/gpl-2.0.html

Unless required by applicable law or agreed to in writing, software distributed under the GPL Licesnse is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the GPL License for the specific language governing permissions and limitations under the GPL License.
