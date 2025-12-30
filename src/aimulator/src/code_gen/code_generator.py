import argparse
import os
import re
import yaml

class YAMLParser:
  """
  Parses the given YAML file.
  """
  def __init__(self, file_path):
    with open(file_path, 'r') as file:
      data = yaml.safe_load(file)

    self.mem_type = data['type']['mem']
    self.pim_type = data['type']['pim']

    self.cmd_spec_map = {}
    header = data['cmd_spec_matrix'][0]
    rows = data['cmd_spec_matrix'][1:]
    for row in rows:
      cmd = row[0]
      self.cmd_spec_map[cmd] = {
        header[i]: row[i] for i in range(1, len(header))
      }

class FileHandler:
  """
  Handles file operations.
  """
  def __init__(self, mem_type):
    self.mem_type = mem_type

  def open_template_file(self):
    """
    Opens a file named "$mem_template.cpp" after finding the file
    by browsing subdirectories of the directory "src/".
    """
    template_file_name = f"{self.mem_type}_template.cpp"
    for root, _, files in os.walk("src"):
      if template_file_name in files:
        file_path = os.path.join(root, template_file_name)
        with open(file_path, 'r') as file:
          return file.read()
    return None
  
  def generate_pim_sim_code(self):
    """
    
    """

class TemplateModifier:
  def __init__(self):
    
  def modify_base():
    
    
  def modify_frontend():
    

  def modify_controller():


  def modify_device():
    

def main():
  parser = argparse.ArgumentPraser()
  parser.add_argument("yaml_file_name",
    help="Name of the YAML config file, assuming the directory is '($pwd)/../configs/$YAML_file_name.yaml'"
  )
  args = parser.parse_args()
  # Construct the file path for the YAML config
  config_file_path = os.path.join(os.path.dirname(__file__), 
                                  "..", "configs", 
                                  f"{args.yaml_file_name}.yaml")
  # Parse the YAML file
  yaml_parser = YAMLParser(config_file_path)

  # Use FileHandler to open the template file
  file_handler = FileHandler(yaml_parser.mem_type)
  template_content = file_handler.open_template_file()
