# Z CAM E2 ZRAW parser <a href="https://ko-fi.com/storyboardcreativity" target="_blank"><img align="right" src="https://cdn.ko-fi.com/cdn/kofi3.png" alt="Buy Me A Coffee" style="height: 41px !important;width: 174px !important;box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;-webkit-box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;" ></a>

This is a free draft ZRAW decoder. This tool allows to extract ZRAW video files to DNG sequences or HEVC bitstreams (according to camera firmware version).

## Community

<a href="https://t.me/zcam_rus" target="_blank"><img src="docs/soc_net_icons/telegram.png" alt="Buy Me A Coffee" style="height: 40px !important;width: 40px !important;box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;-webkit-box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;" ></a>
<a href="https://www.facebook.com/groups/storyboardcreativity" target="_blank"><img src="docs/soc_net_icons/facebook.png" alt="Buy Me A Coffee" style="height: 40px !important;width: 40px !important;box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;-webkit-box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;" ></a>
<a href="https://vk.com/storyboardcreativity" target="_blank"><img src="docs/soc_net_icons/vk.png" alt="Buy Me A Coffee" style="height: 40px !important;width: 40px !important;box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;-webkit-box-shadow: 0px 3px 2px 0px rgba(190, 190, 190, 0.5) !important;" ></a>

## What ZRAW versions are compatible with this software?

For versions before 0.94 you can get pure Bayer CFA, for later versions - you get real HEVC bitstream extracted from ".ZRAW" file without any reencoding process.

### Features

| E2 firmware version | ZRAW contains | Feature |
| :---: |:---:|:---:|
|  0.93  | Compressed RAW CFA 12-bit | Convert ZRAW to DNG sequence |
|  0.94  | HEVC 10-bit 4:2:0 | Extract HEVC-bitstream (`".avc"`) from ZRAW file |
|  0.95  | HEVC 10-bit 4:2:0 | Extract HEVC-bitstream (`".avc"`) from ZRAW file |
|  0.96  | HEVC 10-bit 4:2:0 | Extract HEVC-bitstream (`".avc"`) from ZRAW file |
|  0.97  | HEVC 10-bit 4:2:0 | Extract HEVC-bitstream (`".avc"`) from ZRAW file |
|  0.98  | HEVC 10-bit 4:2:0 | Extract HEVC-bitstream (`".avc"`) from ZRAW file |
| 0.98.1 | HEVC 10-bit 4:2:0 | Extract HEVC-bitstream (`".avc"`) from ZRAW file |

## How to build?

##### For Windows
- Get dependencies for project (OpenSSL, for example)
- Open `Visual Studio Developer Command Prompt`, type `msbuild vc2017/zraw-parser.sln /t:zraw-parser.vcxproj /p:Configuration="Release" /p:Platform="x64"` and have fun!

##### For Ubuntu:
- Get dependencies for project (OpenSSL, for example)
- Type `make` and have fun!

## License

Copyright 2021 storyboardcreativity

This software is created solely on a non-commercial basis to ensure compatibility with ZRAW file formats and is licensed under the GNU General Public License version 2 (the "GPL License").

You may obtain a copy of the GPL License in the LICENSE file, or at:

http://www.gnu.org/licenses/gpl-2.0.html

Unless required by applicable law or agreed to in writing, software distributed under the GPL Licesnse is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the GPL License for the specific language governing permissions and limitations under the GPL License.
