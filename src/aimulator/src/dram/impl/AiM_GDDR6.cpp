#include "dram/AiM_dram.h"
#include "dram/AiM_lambdas.h"

namespace Ramulator {

class GDDR6 : public IDRAM, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IDRAM, GDDR6, "GDDR6", "GDDR6-AiM Device Model")

  public:
    inline static const std::map<std::string, Organization> org_presets = { //Table 19 for more info
      //    name         density   DQ   Ch  Bg Ba  Ro     Co
      {"GDDR6_8Gb_x8",   {8<<10,   8,  {2,  4, 4, 1<<14, 1<<11}}},
      {"GDDR6_8Gb_x16",  {8<<10,   16, {2,  4, 4, 1<<14, 1<<10}}},
      {"GDDR6_16Gb_x8",  {16<<10,  8,  {2,  4, 4, 1<<15, 1<<11}}},
      {"GDDR6_16Gb_x16", {16<<10,  16, {2,  4, 4, 1<<14, 1<<11}}},
      {"GDDR6_32Gb_x8",  {32<<10,  8,  {2,  4, 4, 1<<16, 1<<11}}},
      {"GDDR6_32Gb_x16", {32<<10,  16, {2,  4, 4, 1<<15, 1<<11}}},
      // {"GDDR6_AiM_32Gb", {32<<10, 16, {1,  4, 4, 1<<17, 1<<10}}},
      // 64 GB AiM
      {"GDDR6_AiM_512Gb", {128<<10, 16, {16, 4, 4, 1<<14, 1<<10}}},
    };

    inline static const std::map<std::string, std::vector<int>> timing_presets = {
      //       name                 rate   nBL nCL  nRCDRD nRCDWR  nRP   nRAS  nRC   nWR  nRTP nCWL nCCDS nCCDL nRRDS nRRDL nWTRS nWTRL nFAW  nRFC nRFCpb nRREFD  nREFI  // AiM: nRCDRDMAC  nRCDRDAF  nRCDRDCP  nRCDWRCP  nRCDEWMUL  nCLREG  nCLGB  nCWLREG  nCWLGB  nWPRE  tCK_ps  
      {"GDDR6_2000_1350mV_double", {2000,  8,  24,    26,    16,   26,   53,   79,   26,   4,   6,   4,    6,    7,    7,     9,    11,  28,   210,  105,   14,   3333,             0,         0,        0,        0,        0,        0,     0,      0,      0,      0,     570}},
      {"GDDR6_2000_1250mV_double", {2000,  8,  24,    30,    19,   30,   60,   89,   30,   4,   6,   4,    6,    11,   11,    9,    11,  42,   210,  105,   21,   3333,             0,         0,        0,        0,        0,        0,     0,      0,      0,      0,     570}},
      {"GDDR6_2000_1350mV_quad",   {2000,  4,  24,    26,    16,   26,   53,   79,   26,   4,   6,   4,    6,    7,    7,     9,    11,  28,   210,  105,   14,   3333,             0,         0,        0,        0,        0,        0,     0,      0,      0,      0,     570}},
      {"GDDR6_2000_1250mV_quad",   {2000,  4,  24,    30,    19,   30,   60,   89,   30,   4,   6,   4,    6,    11,   11,    9,    11,  42,   210,  105,   21,   3333,             0,         0,        0,        0,        0,        0,     0,      0,      0,      0,     570}},
      // AiM
      {"GDDR6_AiM",                {2000,  2,  50,    36,    28,   32,   54,   89,   33,   12,  6,   2,    2,    11,   11,    9,    11,  42,   210,  105,   21,   3333,            56,        86,       66,       48,       25,        0,     1,      1,      1,      1,     500}},
    };


  /************************************************
   *                Organization
   ***********************************************/   
    const int m_internal_prefetch_size = 16;

    inline static constexpr ImplDef m_levels = {
      "channel", "bankgroup", "bank", "row", "column",
    };


  /************************************************
   *             Requests & Commands
   ***********************************************/
    inline static constexpr ImplDef m_commands = { //figure 3
      "ACT", 
      "PREA", "PRE",
      "RD",  "WR",  "RDA",  "WRA",
      "REFab", "REFpb", "REFp2b",
      // Single-bank AiM commands leverage the conventional RD, WR, etc.
      "MACSB", "AFSB", "RDCP", "WRCP",
      // Multi-bank AiM commands
      "ACT4_BG", "PRE4_BG", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD",
      // TODO: 4-bank commands for inter-bank group
      // "ACT4_BKS", "ACTAF4_BKS", "PRE4_BKS", "MAC4B_INTER", "AF4B_INTER",
      "ACT16", "MACAB", "AFAB", "WRAFLUT", "WRBK",
      // No-bank AiM commands
      "WRGB", "WRMAC", "WRBIAS", "RDMAC", "RDAF",
    };

    inline static const ImplLUT m_command_addressing_level = LUT (
      m_commands, m_levels, {
        {"ACT", "row"},
        {"PREA", "channel"}, {"PRE", "bank"},
        {"RD", "column"}, {"WR", "column"}, {"RDA", "column"}, {"WRA", "column"},
        {"REFab", "channel"}, {"REFp2b", "channel"}, {"REFpb", "bank"},
        // Single-bank AiM commands
        {"MACSB", "column"}, {"AFSB", "column"}, {"RDCP", "column"}, {"WRCP", "column"},
        // Multi-bank AiM commands
        {"ACT4_BG", "row"}, {"PRE4_BG", "bank"},
        {"MAC4B_INTRA", "column"}, {"AF4B_INTRA", "column"}, {"EWMUL", "column"}, {"EWADD", "column"}, 
        // TODO: 4-bank commands for inter-bank group
        // {"ACT4_BKS", "row"}, {"ACTAF4_BKS", "row"}, {"PRE4_BKS", "bank"},
        // {"MAC4B_INTER", "column"}, {"AF4B_INTER", "column"},
        {"ACT16", "row"}, {"MACAB", "column"},  {"AFAB", "column"},
        {"WRAFLUT", "row"},  {"WRBK", "column"},
        // No-bank AiM commands
        {"WRGB", "channel"}, {"WRMAC", "bank"}, {"WRBIAS", "bank"},
        {"RDMAC", "bank"}, {"RDAF", "bank"},
      }
    );

    inline static const ImplLUT m_command_action_scope = LUT (
      m_commands, m_levels, {
        {"ACT", "row"},
        {"PREA", "channel"}, {"PRE", "bank"},
        {"RD", "column"}, {"WR", "column"}, {"RDA", "column"}, {"WRA", "column"},
        {"REFab", "channel"}, {"REFp2b", "channel"}, {"REFpb", "bank"},
        // Single-bank AiM commands
        {"MACSB", "column"}, {"AFSB", "column"}, {"RDCP", "column"}, {"WRCP", "column"},
        // 4-bank commands for intra-bank group
        {"ACT4_BG", "bankgroup"}, {"PRE4_BG", "bankgroup"},
        {"MAC4B_INTRA", "bankgroup"}, {"AF4B_INTRA", "bankgroup"}, {"EWMUL", "bankgroup"}, {"EWADD", "bankgroup"},
        // TODO: 4-bank commands for inter-bank group
        // {"ACT4_BKS", "bank"}, {"ACTAF4_BKS", "bank"}, {"PRE4_BKS", "bank"},
        // {"MAC4B_INTER", "bank"}, {"AF4B_INTER", "bank"}, {"EWMUL", "bank"}, {"EWADD", "bank"},
        // all-bank commands
        {"ACT16", "channel"}, {"MACAB", "channel"}, {"AFAB", "channel"}, 
        {"WRAFLUT", "channel"}, {"WRBK", "channel"},
        // No-bank AiM commands
        {"WRGB", "channel"}, {"WRMAC", "channel"}, {"WRBIAS", "channel"},
        {"RDMAC", "channel"}, {"RDAF", "channel"},
      }
    );

    inline static const ImplLUT m_command_meta = LUT<DRAMCommandMeta> (
      m_commands, {
                         // open?   close?   access?  refresh?
        {"ACT",            {true,   false,   false,   false}},
        {"PREA",           {false,  true,    false,   false}},
        {"PRE",            {false,  true,    false,   false}},
        {"RD",             {false,  false,   true,    false}},
        {"WR",             {false,  false,   true,    false}},
        {"RDA",            {false,  true,    true,    false}},
        {"WRA",            {false,  true,    true,    false}},
        {"REFab",          {false,  false,   false,   true }}, //double check
        {"REFpb",          {false,  false,   false,   true }},
        {"REFp2b",         {false,  false,   false,   true }},
        // Single-bank AiM commands
        {"MACSB",          {false,  false,   true,    false}},
        {"AFSB",           {false,  false,   true,    false}},
        {"RDCP",           {false,  false,   true,    false}},
        {"WRCP",           {false,  false,   true,    false}},
        // Multi-bank AiM commands
        {"ACT4_BG",        {true,   false,   false,   false}},
        {"PRE4_BG",        {false,  true,    false,   false}},
        {"MAC4B_INTRA",    {false,  false,   true,    false}},
        {"AF4B_INTRA" ,    {false,  false,   true,    false}},
        {"EWMUL",          {false,  false,   true,    false}},
        {"EWADD",          {false,  false,   true,    false}},
        // TODO: 4-bank commands for inter-bank group
        // {"ACT4_BKS",    {true,   false,   false,   false}},
        // {"ACTAF4_BKS",  {true,   false,   false,   false}},
        // {"PRE4_BKS",    {false,  true,    false,   false}},
        // {"MAC4B_INTER", {false,  false,   true,    false}},
        // {"AF4B_INTER",  {false,  false,   true,    false}},
        {"ACT16",          {true,   false,   false,   false}},
        {"MACAB",          {false,  false,   true,    false}},
        {"AFAB" ,          {false,  false,   true,    false}},
        {"WRAFLUT",        {false,  true,    true,    false}},
        {"WRBK",           {false,  false,   true,    false}},
        // AiM DMA commands
        {"WRGB",           {false,  false,   false,   false}},
        {"WRMAC",          {false,  false,   false,   false}},
        {"WRBIAS",         {false,  false,   false,   false}},
        {"RDMAC",          {false,  false,   false,   false}},
        {"RDAF",           {false,  false,   false,   false}},
      }
    );

    inline static constexpr ImplDef m_requests = {
      "read", "write", "all-bank-refresh", "PREsb"
    };

    inline static const ImplLUT m_request_translations = LUT (
      m_requests, m_commands, {
        {"read", "RD"}, {"write", "WR"}, {"all-bank-refresh", "REFab"}, {"PREsb", "PRE"},
      }
    );

    inline static const ImplDef m_aim_req = {
      // Single-bank AiM commands
      "MAC_SBK", "AF_SBK", "COPY_BKGB", "COPY_GBBK",
      // Multi-bank AiM commands
      "MAC_4BK_INTRA_BG", "AF_4BK_INTRA_BG", "EWMUL", "EWADD",
      // TODO: 4-bank commands for inter-bank group
      // "MAC_4BK_INTER_BG", "AF_4BK_INTER_BG",
      "MAC_ABK", "AF_ABK", "WR_AFLUT", "WR_BK",
      // AiM DMA commands
      "WR_GB", "WR_MAC", "WR_BIAS", "RD_MAC", "RD_AF"
    };

    inline static const ImplLUT m_aim_req_translation = LUT (
      m_aim_req, m_commands, {
        // Single-bank AiM commands
        {"MAC_SBK", "MACSB"}, {"AF_SBK", "AFSB"}, {"COPY_BKGB", "RDCP"}, {"COPY_GBBK", "WRCP"},
        // Multi-bank AiM commands
        {"MAC_4BK_INTRA_BG", "MAC4B_INTRA"}, {"AF_4BK_INTRA_BG", "AF4B_INTRA"}, {"EWMUL", "EWMUL"}, {"EWADD", "EWADD"},
        // TODO: 4-bank commands for inter-bank group
        // {"MAC_4BK_INTER_BG", "MAC4B_INTER"}, {"AF_4BK_INTER_BG", "AF4B_INTER"}, 
        {"MAC_ABK", "MACAB"}, {"AF_ABK", "AFAB"}, {"WR_AFLUT", "WRAFLUT"}, {"WR_BK", "WRBK"},
        // AiM DMA commands
        {"WR_GB", "WRGB"}, {"WR_MAC", "WRMAC"}, {"WR_BIAS", "WRBIAS"},
        {"RD_MAC", "RDMAC"}, {"RD_AF", "RDAF"},
      }
    );

  /************************************************
   *                   Timing
   ***********************************************/
  
  // AiM TODO 
  // tFAW is not necessary for ACT4/16

  //delete nCS
    inline static constexpr ImplDef m_timings = {
      "rate", 
      "nBL", "nCL", "nRCDRD", "nRCDWR", "nRP", "nRAS", "nRC", "nWR", "nRTP", "nCWL",
      "nCCDS", "nCCDL",
      "nRRDS", "nRRDL",
      "nWTRS", "nWTRL",
      "nFAW",
      "nRFC","nREFI", "nRREFD", "nRFCpb",
      // AiM
      "nRCDRDMAC", "nRCDRDAF", "nRCDRDCP", "nRCDWRCP", "nRCDEWMUL",
      "nCLGB", "nCLREG", "nCWLGB", "nCWLREG", "nWPRE",
      "tCK_ps",
    };

  /************************************************
   *                 Node States
   ***********************************************/
    inline static constexpr ImplDef m_states = {
      "Opened", "Closed", "PowerUp", "N/A", "Refreshing"
    };

    inline static const ImplLUT m_init_states = LUT (
      m_levels, m_states, {
        {"channel",   "N/A"}, 
        {"bankgroup", "N/A"},
        {"bank",      "Closed"},
        {"row",       "Closed"},
        {"column",    "N/A"},
      }
    );

  public:
    struct Node : public DRAMNodeBase<GDDR6> {
      Node(GDDR6* dram, Node* parent, int level, int id) : DRAMNodeBase<GDDR6>(dram, parent, level, id) {};
    };
    std::vector<Node*> m_channels;

    FuncMatrix<ActionFunc_t<Node>>  m_actions;
    FuncMatrix<PreqFunc_t<Node>>    m_preqs;
    FuncMatrix<RowhitFunc_t<Node>>  m_rowhits;
    FuncMatrix<RowopenFunc_t<Node>> m_rowopens;

  public:
    void tick() override { m_clk++; };

    void init() override {
      RAMULATOR_DECLARE_SPECS();
      set_organization();
      set_timing_vals();

      set_actions();
      set_preqs();
      set_rowhits();
      set_rowopens();
      
      create_nodes();
    };

    void issue_command(int command, const AddrHierarchy_t& addr_h) override {
      int channel_id = addr_h[m_levels["channel"]];
      m_channels[channel_id]->update_timing(command, addr_h, m_clk);
      m_channels[channel_id]->update_states(command, addr_h, m_clk);
      switch (command)
      {
      case m_commands["PREA"]: {
        m_open_rows[channel_id] = 0;
        break;
      }
      case m_commands["PRE4_BG"]: {
        int bankgroup_id = addr_h[m_levels["bankgroup"]];
        for (int bank_id = bankgroup_id * 4; bank_id < (bankgroup_id + 1) * 4; bank_id++) {
          m_open_rows[channel_id] &= ~((uint16_t)(1 << bank_id));
        }
        break;
      }
      case m_commands["PRE"]:
      case m_commands["RDA"]:
      case m_commands["WRA"]: {
        int bank_id = addr_h[m_levels["bank"]];
        m_open_rows[channel_id] &= ~((uint16_t)(1 << bank_id));
        break;
      }
      case m_commands["ACT16"]: {
        m_open_rows[channel_id] = (uint16_t)(0xFFFF);
        break;
      }
      case m_commands["ACT4_BG"]: {
        int bankgroup_id = addr_h[m_levels["bankgroup"]];
        for (int bank_id = bankgroup_id * 4; bank_id < (bankgroup_id + 1) * 4; bank_id++) {
          m_open_rows[channel_id] |= ((uint16_t)(1 << bank_id));
        }
        break;
      }
      case m_commands["ACT"]: {
        int bank_id = addr_h[m_levels["bank"]];
        m_open_rows[channel_id] |= (uint16_t)(1 << bank_id);
        break;
      }
      default:
        break;
      }
    };

    int get_preq_command(int command, const AddrHierarchy_t& addr_h) override {
      int channel_id = addr_h[m_levels["channel"]];
      return m_channels[channel_id]->get_preq_command(command, addr_h, m_clk);
    };

    bool check_ready(int command, const AddrHierarchy_t& addr_h) override {
      int channel_id = addr_h[m_levels["channel"]];
      return m_channels[channel_id]->check_ready(command, addr_h, m_clk);
    };

    bool check_rowbuffer_hit(int command, const AddrHierarchy_t& addr_h) override {
      int channel_id = addr_h[m_levels["channel"]];
      return m_channels[channel_id]->check_rowbuffer_hit(command, addr_h, m_clk);
    };
    
    bool check_node_open(int command, const AddrHierarchy_t& addr_h) override {
      int channel_id = addr_h[m_levels["channel"]];
      return m_channels[channel_id]->check_node_open(command, addr_h, m_clk);
    };

  private:
    void set_organization() {
      // Channel width
      m_channel_width = param_group("org").param<int>("channel_width").default_val(64);

      // Organization
      m_organization.count.resize(m_levels.size(), -1);

      // Load organization preset if provided
      if (auto preset_name = param_group("org").param<std::string>("preset").optional()) {
        if (org_presets.count(*preset_name) > 0) {
          m_organization = org_presets.at(*preset_name);
        } else {
          throw ConfigurationError("Unrecognized organization preset \"{}\" in {}!", *preset_name, get_name());
        }
      }

      // Override the preset with any provided settings
      if (auto dq = param_group("org").param<int>("dq").optional()) {
        m_organization.dq = *dq;
      }

      for (int i = 0; i < m_levels.size(); i++){
        auto level_name = m_levels(i);
        if (auto sz = param_group("org").param<int>(level_name).optional()) {
          m_organization.count[i] = *sz;
        }
      }

      if (auto density = param_group("org").param<int>("density").optional()) {
        m_organization.density = *density;
      }

      // Sanity check: is the calculated chip density the same as the provided one?
      size_t _density = size_t(m_organization.count[m_levels["channel"]]) *
                        size_t(m_organization.count[m_levels["bankgroup"]]) *
                        size_t(m_organization.count[m_levels["bank"]]) *
                        size_t(m_organization.count[m_levels["row"]]) *
                        size_t(m_organization.count[m_levels["column"]]) *
                        size_t(m_organization.dq);
      _density >>= 20;
      if (m_organization.density != _density) {
        throw ConfigurationError(
          "Calculated {} chip density {} Mb does not equal the provided density {} Mb!", 
          get_name(),
          _density, 
          m_organization.density
        );
      }
    };

    void set_timing_vals() {
      m_timing_vals.resize(m_timings.size(), -1);
      m_command_latencies.resize(m_commands.size(), -1);

      // Load timing preset if provided
      bool preset_provided = false;
      if (auto preset_name = param_group("timing").param<std::string>("preset").optional()) {
        if (timing_presets.count(*preset_name) > 0) {
          m_timing_vals = timing_presets.at(*preset_name);
          preset_provided = true;
        } else {
          throw ConfigurationError("Unrecognized timing preset \"{}\" in {}!", *preset_name, get_name());
        }
      }

      // Check for rate (in MT/s), and if provided, calculate and set tCK (in picosecond)
      if (auto dq = param_group("timing").param<int>("rate").optional()) {
        if (preset_provided) {
          throw ConfigurationError("Cannot change the transfer rate of {} when using a speed preset !", get_name());
        }
        m_timing_vals("rate") = *dq;
      }
      int tCK_ps = 1E6 / (m_timing_vals("rate") / 2);
      m_timing_vals("tCK_ps") = tCK_ps;

      // Load the organization specific timings
      int dq_id = [](int dq) -> int {
        switch (dq) {
          case 8:  return 0;
          case 16: return 1;
          default: return -1;
        }
      }(m_organization.dq);

      int rate_id = [](int rate) -> int { //should low voltage operation be added here?
        switch (rate) {
          case 2000: return  0;
          default:   return -1;
        }
      }(m_timing_vals("rate"));

      // Tables for secondary timings determined by the frequency, density, and DQ width.
      // Defined in the JEDEC standard (e.g., Table 169-170, JESD79-4C).

      //update these values
      constexpr int nRRDS_TABLE[2][1] = {
      // 2000
        { 4 },   // x8
        { 5 },   // x16
      };
      constexpr int nRRDL_TABLE[2][1] = {
      // 2000
        { 5 },  // x8
        { 6 },  // x16
      };
      constexpr int nFAW_TABLE[2][1] = {
      // 2000
        { 20 },  // x8
        { 28 },  // x16
      };

      if (dq_id != -1 && rate_id != -1) {
        // Organizatino-dependent timings
        m_timing_vals("nRRDS") = nRRDS_TABLE[dq_id][rate_id];
        m_timing_vals("nRRDL") = nRRDL_TABLE[dq_id][rate_id];
        m_timing_vals("nFAW")  = nFAW_TABLE [dq_id][rate_id];
      }

      // Refresh timings
      // tRFC table (unit is nanosecond!)
      constexpr int tRFC_TABLE[3][3] = {
      //  4Gb   8Gb  16Gb
        { 260,  360,  550}, // Normal refresh (tRFC1)
        { 160,  260,  350}, // FGR 2x (tRFC2)
        { 110,  160,  260}, // FGR 4x (tRFC4)
      };

      // tREFI(base) table (unit is nanosecond!)
      constexpr int tREFI_BASE = 7800;
      int density_id = [](int density_Mb) -> int { 
        switch (density_Mb) {
          case 4096:  return 0;
          case 8192:  return 1;
          case 16384: return 2;
          default:    return -1;
        }
      }(m_organization.density);

      // Density-dependent timings
      m_timing_vals("nRFC")  = JEDEC_rounding(tRFC_TABLE[0][density_id], tCK_ps);
      m_timing_vals("nREFI") = JEDEC_rounding(tREFI_BASE, tCK_ps);

      // Overwrite timing parameters with any user-provided value
      // Rate and tCK should not be overwritten
      for (int i = 1; i < m_timings.size() - 1; i++) {
        auto timing_name = std::string(m_timings(i));

        if (auto provided_timing = param_group("timing").param<int>(timing_name).optional()) {
          // Check if the user specifies in the number of cycles (e.g., nRCD)
          m_timing_vals(i) = *provided_timing;
        } else if (auto provided_timing = param_group("timing").param<float>(timing_name.replace(0, 1, "t")).optional()) {
          // Check if the user specifies in nanoseconds (e.g., tRCD)
          m_timing_vals(i) = JEDEC_rounding(*provided_timing, tCK_ps);
        }
      }

      // Check if there is any uninitialized timings
      for (int i = 0; i < m_timing_vals.size(); i++) {
        if (m_timing_vals(i) == -1) {
          throw ConfigurationError("In \"{}\", timing {} is not specified!", get_name(), m_timings(i));
        }
      }

      // Set execution time (completion latency) for the commands
      m_command_latencies("RD") = m_timing_vals("nCL") + m_timing_vals("nBL");
      m_command_latencies("WR") = m_timing_vals("nCWL") + m_timing_vals("nWR");
      
      // AiM commands
      m_command_latencies("MACSB") = 1;
      m_command_latencies("AFSB")  = 1;
      m_command_latencies("RDCP")  = 1;
      m_command_latencies("WRCP")  = 1;
      m_command_latencies("MAC4B_INTRA") = 1;
      m_command_latencies("AF4B_INTRA")  = 1;
      m_command_latencies("MACAB") = 1;
      m_command_latencies("AFAB")  = 1;
      m_command_latencies("EWMUL") = 1;
      m_command_latencies("EWADD") = 1;
      m_command_latencies("WRBK")  = 1;
      m_command_latencies("WRGB")  = m_timing_vals("nCWLGB")  + m_timing_vals("nBL");
      m_command_latencies("WRMAC") = m_timing_vals("nCWLREG") + m_timing_vals("nBL");
      m_command_latencies("WRBIAS") = m_timing_vals("nCWLREG") + m_timing_vals("nBL");
      m_command_latencies("RDMAC") = m_timing_vals("nCLREG")  + m_timing_vals("nBL");
      m_command_latencies("RDAF")  = m_timing_vals("nCLREG")  + m_timing_vals("nBL");      
      // AiM timings
      m_timing_vals("nCLREG")  = 0;
      m_timing_vals("nCLGB")   = 1;
      m_timing_vals("nCWLREG") = 1;
      m_timing_vals("nCWLGB")  = 1;
      m_timing_vals("nWPRE")   = 1;

      // Populate the timing constraints
      #define V(timing) (m_timing_vals(timing))
      populate_timingcons(this, {
        /****************************************************************************************************
         *                                            Channel
         ***************************************************************************************************/
        /************************************************************
         * CAS -> CAS
         * External data bus occupancy
         * AiM commands that transfer data on the external bus
         ************************************************************/
        {.level = "channel", .preceding = {"RD", "RDA"}, .following = {"RD", "RDA"}, .latency = V("nBL")},
        {.level = "channel", .preceding = {"WR", "WRA"}, .following = {"WR", "WRA"}, .latency = V("nBL")},
        {.level = "channel", .preceding = {"MACSB", "AFSB"}, .following = {"WR", "WRA"}, .latency = V("nBL")},
        // AiM; TODO: Which component stores the result? Which command stores the result?
        /****************************************************************************************************
         *                                     DIFFERENT BankGroup (Rank)
         * Changed from rank to channel, some duplicates, what takes
         ***************************************************************************************************/
        /************************************************************
         * RAS -> RAS
         *************************************************************/
        // ACT <-> ACT
        {.level = "channel", .preceding = {"ACT16"}, .following = {"ACT16"}, .latency = V("nRRDS")},
        {.level = "channel", .preceding = {"ACT4_BG"}, .following = {"ACT", "ACT4_BG"}, .latency = V("nRRDS")},
        {.level = "channel", .preceding = {"ACT"}, .following = {"ACT4_BG"}, .latency = V("nRRDS")},
        // ACT <-> PRE
        {.level = "channel", .preceding = {"ACT", "ACT4_BG", "ACT16"}, .following = {"PREA"}, .latency = V("nRAS")},
        {.level = "channel", .preceding = {"ACT16"}, .following = {"PRE4_BG"}, .latency = V("nRAS")},
        {.level = "channel", .preceding = {"PRE", "PRE4_BG", "PREA"}, .following = {"ACT16"}, .latency = V("nRP")},
        {.level = "channel", .preceding = {"PRE4_BG", "PREA"}, .following = {"ACT"}, .latency = V("nRP")},
        /************************************************************
         * CAS <-> RAS
         *************************************************************/
        {.level = "channel", .preceding = {"ACT16"}, .following = {"RD", "RDA", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "EWMUL", "EWADD", "MACAB"}, .latency = V("nRCDRD")},
        {.level = "channel", .preceding = {"ACT16"}, .following = {"WR", "WRA", "WRCP", "WRBK"}, .latency = V("nRCDWR")},
        {.level = "channel", .preceding = {"RD", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"PREA"}, .latency = V("nRTP")},
        {.level = "channel", .preceding = {"WR", "WRCP", "WRAFLUT", "WRBK"}, .following = {"PREA"}, .latency = V("nCWL")+V("nBL")+V("nWR")},
        {.level = "channel", .preceding = {"MACAB", "AFAB"}, .following = {"PRE4_BG"}, .latency = V("nRTP")},
        {.level = "channel", .preceding = {"RDA"}, .following = {"ACT16"}, .latency = V("nRTP")+V("nRP")},
        {.level = "channel", .preceding = {"WRA"}, .following = {"ACT16"}, .latency = V("nCWL")+V("nBL")+V("nWR")+V("nRP")},
        /************************************************************
         * CAS -> CAS
         * nCCDS is the minimal latency between two successive distinct column commands that access to DIFFERENT bankgroups
         * AiM commands that transfer data on the bus shared between bankgroups
         * - AiM RD: MACAB, AFAB, MAC4B_INTRA, AF4B_INTRA, MACSB, AFSB
         * - AiM WR: WRCP
         ************************************************************/
        {.level = "channel", .preceding = {"RD", "RDA", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"RD", "RDA", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .latency = V("nCCDS")},
        {.level = "channel", .preceding = {"WR", "WRA", "WRCP", "WRAFLUT", "WRBK"}, .following = {"WR", "WRA", "WRCP", "WRAFLUT", "WRBK"}, .latency = V("nCCDS")},
        /************************************************************
         * RD -> WR, Assuming tWPRE = 1 tCK
         * Minimum Read to Write, tRTW
         *************************************************************/
        // + 1 for assuming bus turn around time
        {.level = "channel", .preceding = {"RD", "RDA", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"WR", "WRA", "WRCP", "WRAFLUT", "WRBK"}, .latency = V("nCL")+V("nBL")+3-V("nCWL")+V("nWPRE")},
        // No-bank
        // {.level = "channel", .preceding = {"RDMAC, RDAF"}, .following = {"WR", "WRA", "WRCP"}, .latency = V("nCLREG")+V("nBL")+3-V("nCWL")+V("nWPRE")},
        // {.level = "channel", .preceding = {"RDMAC, RDAF"}, .following = {"WRGB"}, .latency = V("nCLREG")+V("nBL")+3-V("nCWLGB")+V("nWPRE")},
        // {.level = "channel", .preceding = {"RDMAC, RDAF"}, .following = {"WRMAC"}, .latency = V("nCLREG")+V("nBL")+3-V("nCWLREG")+V("nWPRE")},
        /************************************************************
         * WR -> RD
         * Minimum Read after Write
         *************************************************************/
        {.level = "channel", .preceding = {"WR", "WRA", "WRCP", "WRAFLUT", "WRBK"}, .following = {"RD", "RDA", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .latency = V("nCWL")+V("nBL")+V("nWTRS")},
        // No-bank
        // {.level = "channel", .preceding = {"WRGB"}, .following = {"RD", "RDA", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "AF4B_INTRA", "MACAB", "AFAB"}, .latency = V("nCWLGB")+V("nBL")+V("nWTRS")},
        // {.level = "channel", .preceding = {"WRMAC"}, .following = {"RD", "RDA", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "AF4B_INTRA", "MACAB", "AFAB"}, .latency = V("nCWLREG")+V("nBL")+V("nWTRS")},
        /************************************************************
         * RAS -> REF
         *************************************************************/
        {.level = "channel", .preceding = {"ACT", "ACT4_BG", "ACT16"}, .following = {"REFab"}, .latency = V("nRC")},
        {.level = "channel", .preceding = {"PRE", "PRE4_BG", "PREA"}, .following = {"REFab"}, .latency = V("nRP")},
        {.level = "channel", .preceding = {"RDA"}, .following = {"REFab"}, .latency = V("nRP")+V("nRTP")},
        {.level = "channel", .preceding = {"WRA"}, .following = {"REFab"}, .latency = V("nCWL")+V("nBL")+V("nWR")+V("nRP")},
        {.level = "channel", .preceding = {"REFab"}, .following = {"ACT", "ACT4_BG", "ACT16"}, .latency = V("nRFC")},
        /************************************************************
         * RAS -> REFp2b
         *************************************************************/
        {.level = "channel", .preceding = {"ACT"}, .following = {"REFp2b"}, .latency = V("nRRDL")},
        {.level = "channel", .preceding = {"PRE"}, .following = {"REFp2b"}, .latency = V("nRP")},
        {.level = "channel", .preceding = {"RDA"}, .following = {"REFp2b"}, .latency = V("nRP")+V("nRTP")},
        {.level = "channel", .preceding = {"WRA"}, .following = {"REFp2b"}, .latency = V("nCWL")+V("nBL")+V("nWR")+V("nRP")},
        {.level = "channel", .preceding = {"REFp2b"}, .following = {"ACT"}, .latency = V("nRREFD")},
        /************************************************************
         * RAS -> REFpb
         *************************************************************/
        {.level = "channel", .preceding = {"ACT"}, .following = {"REFpb"}, .latency = V("nRRDL")},
        {.level = "channel", .preceding = {"PRE"}, .following = {"REFpb"}, .latency = V("nRP")},
        {.level = "channel", .preceding = {"RDA"}, .following = {"REFpb"}, .latency = V("nRP")+V("nRTP")},
        {.level = "channel", .preceding = {"WRA"}, .following = {"REFpb"}, .latency = V("nCWL")+V("nBL")+V("nWR")+V("nRP")},
        {.level = "channel", .preceding = {"REFpb"}, .following = {"ACT"}, .latency = V("nRREFD")},
        /****************************************************************************************************
         *                                         SAME Bankgroup
         ***************************************************************************************************/ 
        /************************************************************
         * RAS -> RAS
         * AiM RAS: ACT4, PRE4_BG
         *************************************************************/
        {.level = "bankgroup", .preceding = {"ACT"}, .following = {"ACT"}, .latency = V("nRRDL")},
        {.level = "bankgroup", .preceding = {"ACT", "ACT4_BG"}, .following = {"PRE4_BG"}, .latency = V("nRAS")},
        {.level = "bankgroup", .preceding = {"PRE", "PRE4_BG", "PREA"}, .following = {"ACT4_BG"}, .latency = V("nRP")},
        /************************************************************
         * CAS <-> RAS
         ************************************************************/
        {.level = "bankgroup", .preceding = {"ACT4_BG"}, .following = {"RD", "RDA", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "EWMUL", "EWADD"}, .latency = V("nRCDRD")},
        {.level = "bankgroup", .preceding = {"ACT4_BG"}, .following = {"WR", "WRA", "WRCP"}, .latency = V("nRCDWR")},
        {.level = "bankgroup", .preceding = {"RD", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD"}, .following = {"PRE4_BG"}, .latency = V("nRTP")},
        {.level = "bankgroup", .preceding = {"WR", "WRCP", "WRAFLUT", "WRBK"}, .following = {"PRE4_BG"}, .latency = V("nCWL")+V("nBL")+V("nWR")},
        {.level = "bankgroup", .preceding = {"RDA"}, .following = {"ACT4_BG"}, .latency = V("nRTP")+V("nRP")},
        {.level = "bankgroup", .preceding = {"WRA"}, .following = {"ACT4_BG"}, .latency = V("nCWL")+V("nBL")+V("nWR")+V("nRP")},
        /************************************************************
         * CAS -> CAS
         * nCCDL is the minimal latency between two successive distinct column commands that access to the SAME bankgroup
         * AiM commands that transfer data on the bus within the bankgroup
         * AiM CAS: MAC4B_INTRA, AF4B_INTRA, MACSB, AFSB, RDCP, WRCP
         *************************************************************/
        {.level = "bankgroup", .preceding = {"RD", "RDA", "MACSB", "AFSB", "RDCP"}, .following = {"RD", "RDA", "MACSB", "AFSB", "RDCP"}, .latency = V("nCCDL")},
        {.level = "bankgroup", .preceding = {"WR", "WRA", "WRCP"}, .following = {"WR", "WRA", "WRCP"}, .latency = V("nCCDL")},
        // WR -> RD
        {.level = "bankgroup", .preceding = {"WR", "WRA", "WRCP"}, .following = {"RD", "RDA", "MACSB", "AFSB", "RDCP"}, .latency = V("nCWL")+V("nBL")+V("nWTRL")},
        /****************************************************************************************************
         *                                             Bank
         ***************************************************************************************************/ 
        /************************************************************
         * RAS -> RAS
         * A minimum time, tRAS, must have elapsed between opening and closing a row.
         *************************************************************/
        {.level = "bank", .preceding = {"ACT", "ACT4_BG", "ACT16"}, .following = {"PRE"}, .latency = V("nRAS")},
        {.level = "bank", .preceding = {"PRE"}, .following = {"ACT"}, .latency = V("nRP")},
         /************************************************************
         * CAS -> RAS
         * An ACTIVATE (ACT) command is required to be issued before the READ command to the same bank, and tRCDRD must be met.
         *************************************************************/
        {.level = "bank", .preceding = {"ACT"}, .following = {"RD", "RDA", "MACSB", "AFSB", "RDCP"}, .latency = V("nRCDRD")},
        {.level = "bank", .preceding = {"ACT"}, .following = {"WR", "WRA", "WRCP"},                  .latency = V("nRCDWR")},
        {.level = "bank", .preceding = {"RD", "MACSB", "AFSB", "RDCP", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"PRE"}, .latency = V("nRTP")},
        {.level = "bank", .preceding = {"WR", "WRCP", "WRAFLUT", "WRBK"}, .following = {"PRE"}, .latency = V("nCWL")+V("nBL")+V("nWR")},
        {.level = "bank", .preceding = {"RDA"}, .following = {"ACT"}, .latency = V("nRTP")+V("nRP")},
        {.level = "bank", .preceding = {"WRA"}, .following = {"ACT"}, .latency = V("nCWL")+V("nBL")+V("nWR")+V("nRP")},
        /************************************************************
         * RAS -> REFpb
         * The selected bank must be precharged prior to the REFpb command.
         *************************************************************/
        {.level = "bank", .preceding = {"ACT", "ACT4_BG", "ACT16"}, .following = {"REFpb"}, .latency = V("nRC")},
        {.level = "bank", .preceding = {"PRE", "PRE4_BG", "PREA"}, .following = {"REFpb"}, .latency = V("nRP")},
        {.level = "bank", .preceding = {"RDA"}, .following = {"REFpb"}, .latency = V("nRP")+V("nRTP")},
        {.level = "bank", .preceding = {"WRA"}, .following = {"REFpb"}, .latency = V("nCWL")+V("nBL")+V("nWR")+V("nRP")},
        {.level = "bank", .preceding = {"REFpb"}, .following = {"ACT", "ACT4_BG", "ACT16"}, .latency = V("nRFCpb")},
      });
      #undef V
    };

    void set_actions() {
      m_actions.resize(m_levels.size(), std::vector<ActionFunc_t<Node>>(m_commands.size()));

      // Channel Actions 
      m_actions[m_levels["channel"]][m_commands["PREA"]]  = Lambdas::Action::Channel::PREab<GDDR6>;
      m_actions[m_levels["channel"]][m_commands["REFab"]] = Lambdas::Action::Channel::REFab<GDDR6>;
      // Channel actions; AiM
      m_actions[m_levels["channel"]][m_commands["ACT16"]] = Lambdas::Action::Channel::ACTab<GDDR6>;

      // Bankgroup actions; AiM
      m_actions[m_levels["bankgroup"]][m_commands["PRE4_BG"]] = Lambdas::Action::BankGroup::PRE4b_bg<GDDR6>;
      m_actions[m_levels["bankgroup"]][m_commands["ACT4_BG"]] = Lambdas::Action::BankGroup::ACT4b_bg<GDDR6>;

      // Bank actions
      m_actions[m_levels["bank"]][m_commands["ACT"]] = Lambdas::Action::Bank::ACT<GDDR6>;
      m_actions[m_levels["bank"]][m_commands["PRE"]] = Lambdas::Action::Bank::PRE<GDDR6>;
      m_actions[m_levels["bank"]][m_commands["RDA"]] = Lambdas::Action::Bank::PRE<GDDR6>;
      m_actions[m_levels["bank"]][m_commands["WRA"]] = Lambdas::Action::Bank::PRE<GDDR6>;
    };

    void set_preqs() {
      m_preqs.resize(m_levels.size(), std::vector<PreqFunc_t<Node>>(m_commands.size()));

      // Channel actions; ramulator
      m_preqs[m_levels["channel"]][m_commands["REFab"]] = Lambdas::Preq::Channel::RequireAllBanksClosed<GDDR6>; 
      // Channel actions; AiM all-bank
      m_preqs[m_levels["channel"]][m_commands["MACAB"]]   = Lambdas::Preq::Channel::RequireAllRowsOpen<GDDR6>;
      m_preqs[m_levels["channel"]][m_commands["AFAB"]]    = Lambdas::Preq::Channel::RequireAllRowsOpen<GDDR6>;
      m_preqs[m_levels["channel"]][m_commands["WRAFLUT"]] = Lambdas::Preq::Channel::RequireAllRowsOpen<GDDR6>;
      m_preqs[m_levels["channel"]][m_commands["WRBK"]]    = Lambdas::Preq::Channel::RequireAllRowsOpen<GDDR6>;

      // Bankgroup actions; AiM 4-bank
      m_preqs[m_levels["bankgroup"]][m_commands["MAC4B_INTRA"]] = Lambdas::Preq::BankGroup::RequireAllRowsOpen<GDDR6>;
      m_preqs[m_levels["bankgroup"]][m_commands["AF4B_INTRA"]]  = Lambdas::Preq::BankGroup::RequireAllRowsOpen<GDDR6>;
      m_preqs[m_levels["bankgroup"]][m_commands["EWMUL"]] = Lambdas::Preq::BankGroup::RequireAllRowsOpen<GDDR6>;
      m_preqs[m_levels["bankgroup"]][m_commands["EWADD"]] = Lambdas::Preq::BankGroup::RequireAllRowsOpen<GDDR6>;

      // Bank actions; ramulator
      m_preqs[m_levels["bank"]][m_commands["RD"]]  = Lambdas::Preq::Bank::RequireRowOpen<GDDR6>;
      m_preqs[m_levels["bank"]][m_commands["WR"]]  = Lambdas::Preq::Bank::RequireRowOpen<GDDR6>;
      m_preqs[m_levels["bank"]][m_commands["RDA"]] = Lambdas::Preq::Bank::RequireRowOpen<GDDR6>;
      m_preqs[m_levels["bank"]][m_commands["WRA"]] = Lambdas::Preq::Bank::RequireRowOpen<GDDR6>;
      // Bank actions; AiM single bank
      m_preqs[m_levels["bank"]][m_commands["MACSB"]] = Lambdas::Preq::Bank::RequireRowOpen<GDDR6>;
      m_preqs[m_levels["bank"]][m_commands["AFSB"]]  = Lambdas::Preq::Bank::RequireRowOpen<GDDR6>;
      m_preqs[m_levels["bank"]][m_commands["RDCP"]]  = Lambdas::Preq::Bank::RequireRowOpen<GDDR6>;
      m_preqs[m_levels["bank"]][m_commands["WRCP"]]  = Lambdas::Preq::Bank::RequireRowOpen<GDDR6>;
      
      // can RequireSameBanksClosed be used, or is RequireBankClosed needed?
      //m_preqs[m_levels["channel"]][m_commands["REFpb"]] = Lambdas::Preq::Bank::RequireAllBanksClosed<GDDR6>;
      //m_preqs[m_levels["channel"]][m_commands["REFp2b"]] = Lambdas::Preq::Bank::RequireAllBanksClosed<GDDR6>; 

    };

    void set_rowhits() {
      m_rowhits.resize(m_levels.size(), std::vector<RowhitFunc_t<Node>>(m_commands.size()));

      // ramulator2
      m_rowhits[m_levels["bank"]][m_commands["RD"]] = Lambdas::RowHit::Bank::RDWR<GDDR6>;
      m_rowhits[m_levels["bank"]][m_commands["WR"]] = Lambdas::RowHit::Bank::RDWR<GDDR6>;
      
      // AiM all-bank
      m_rowhits[m_levels["channel"]][m_commands["MACAB"]]   = Lambdas::RowHit::Channel::RDWR16<GDDR6>;
      m_rowhits[m_levels["channel"]][m_commands["AFAB"]]    = Lambdas::RowHit::Channel::RDWR16<GDDR6>;
      m_rowhits[m_levels["channel"]][m_commands["WRAFLUT"]] = Lambdas::RowHit::Channel::RDWR16<GDDR6>;
      m_rowhits[m_levels["channel"]][m_commands["WRBK"]]    = Lambdas::RowHit::Channel::RDWR16<GDDR6>;

      // AiM four-bank
      m_rowhits[m_levels["bankgroup"]][m_commands["MAC4B_INTRA"]] = Lambdas::RowHit::BankGroup::RD4<GDDR6>;
      m_rowhits[m_levels["bankgroup"]][m_commands["AF4B_INTRA"]]  = Lambdas::RowHit::BankGroup::RD4<GDDR6>;
      m_rowhits[m_levels["bankgroup"]][m_commands["EWMUL"]]       = Lambdas::RowHit::BankGroup::RD4<GDDR6>;
      m_rowhits[m_levels["bankgroup"]][m_commands["EWADD"]]       = Lambdas::RowHit::BankGroup::RD4<GDDR6>;

      // AiM single bank
      m_rowhits[m_levels["bank"]][m_commands["MACSB"]] = Lambdas::RowHit::Bank::RDWR<GDDR6>;
      m_rowhits[m_levels["bank"]][m_commands["AFSB"]]  = Lambdas::RowHit::Bank::RDWR<GDDR6>;
      m_rowhits[m_levels["bank"]][m_commands["RDCP"]]  = Lambdas::RowHit::Bank::RDWR<GDDR6>;
      m_rowhits[m_levels["bank"]][m_commands["WRCP"]]  = Lambdas::RowHit::Bank::RDWR<GDDR6>;
    }

    void set_rowopens() {
      m_rowopens.resize(m_levels.size(), std::vector<RowhitFunc_t<Node>>(m_commands.size()));

      // ramulator 2
      m_rowopens[m_levels["bank"]][m_commands["RD"]] = Lambdas::RowOpen::Bank::RDWR<GDDR6>;
      m_rowopens[m_levels["bank"]][m_commands["WR"]] = Lambdas::RowOpen::Bank::RDWR<GDDR6>;

      // AiM all-bank
      m_rowopens[m_levels["channel"]][m_commands["MACAB"]]   = Lambdas::RowOpen::Channel::RDWR16<GDDR6>;
      m_rowopens[m_levels["channel"]][m_commands["AFAB"]]    = Lambdas::RowOpen::Channel::RDWR16<GDDR6>;
      m_rowopens[m_levels["channel"]][m_commands["WRAFLUT"]] = Lambdas::RowOpen::Channel::RDWR16<GDDR6>;
      m_rowopens[m_levels["channel"]][m_commands["WRBK"]]    = Lambdas::RowOpen::Channel::RDWR16<GDDR6>;

      // AiM four-bank
      m_rowopens[m_levels["bankgroup"]][m_commands["MAC4B_INTRA"]] = Lambdas::RowOpen::BankGroup::RD4<GDDR6>;
      m_rowopens[m_levels["bankgroup"]][m_commands["AF4B_INTRA"]]  = Lambdas::RowOpen::BankGroup::RD4<GDDR6>;
      m_rowopens[m_levels["bankgroup"]][m_commands["EWMUL"]]       = Lambdas::RowOpen::BankGroup::RD4<GDDR6>;
      m_rowopens[m_levels["bankgroup"]][m_commands["EWADD"]]       = Lambdas::RowOpen::BankGroup::RD4<GDDR6>;

      // AiM single bank
      m_rowopens[m_levels["bank"]][m_commands["MACSB"]] = Lambdas::RowOpen::Bank::RDWR<GDDR6>;
      m_rowopens[m_levels["bank"]][m_commands["AFSB"]]  = Lambdas::RowOpen::Bank::RDWR<GDDR6>;
      m_rowopens[m_levels["bank"]][m_commands["RDCP"]]  = Lambdas::RowOpen::Bank::RDWR<GDDR6>;
      m_rowopens[m_levels["bank"]][m_commands["WRCP"]]  = Lambdas::RowOpen::Bank::RDWR<GDDR6>;
    }

    void create_nodes() {
      int num_channels = m_organization.count[m_levels["channel"]];
      for (int i = 0; i < num_channels; i++) {
        Node* channel = new Node(this, nullptr, 0, i);
        m_channels.push_back(channel);
        m_open_rows.push_back(0);
      }
    };
};
}        // namespace Ramulator