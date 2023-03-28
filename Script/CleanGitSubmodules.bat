@echo off

cd ..\Source\External\GitSubmodules
REM Iterate over each directory in the current folder
for /d %%f in (*) do (
  echo "Resetting Git workspace in %%f"
  cd "%%f"
  
  REM Reset workspace to last committed state
  git reset --hard HEAD
  
  REM Remove untracked files and directories
  git clean -f -d
  
  cd ..
)

echo "Done!"