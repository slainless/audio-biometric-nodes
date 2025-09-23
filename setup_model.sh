mkdir -p ./.models
cd ./.models

# sudo apt-get install git-lfs

GIT_LFS_SKIP_SMUDGE=1 git clone https://huggingface.co/bookbot/sherpa-ncnn-pruned-transducer-stateless7-streaming-id
cd ./sherpa-ncnn-pruned-transducer-stateless7-streaming-id
git lfs pull --include "*.bin"