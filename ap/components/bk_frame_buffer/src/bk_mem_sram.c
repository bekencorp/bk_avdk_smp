//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <os/os.h>
#include <os/mem.h>
#include <components/bk_hardware_ram.h>


void *bk_get_isp_flexa_buffer(uint32_t size)
{
    return hsram_malloc(size);//os_malloc(size);
}

void *bk_get_gpu_flexa_buffer(uint32_t size)
{
    return hsram_malloc(size);
}

void *bk_get_gpu_output_buffer(uint32_t size)
{
    return hsram_malloc(size);
}

void *bk_get_dpu_buffer(uint32_t size)
{
    return hsram_malloc(size);
}
