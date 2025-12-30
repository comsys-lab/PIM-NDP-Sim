#include "dram/AiM_dram.h"
#include "dram/AiM_lambdas.h"

namespace Ramulator {

class LPDDR5 : public IDRAM, public Implementation {
  RAMULATOR_REGISTER_IMPLEMENTATION(IDRAM, LPDDR5, "LPDDR5", "LPDDR5-AiM Device Model")

  public:
    inline static const std::map<std::string, Organization> org_presets = {
      //   name           density   DQ   Ch Ra Bg Ba   Ro     Co
      {"LPDDR5_2Gb_x16",  {2<<10,   16, { 1, 1, 4, 4, 1<<13, 1<<10}}},
      {"LPDDR5_4Gb_x16",  {4<<10,   16, { 1, 1, 4, 4, 1<<14, 1<<10}}},
      {"LPDDR5_8Gb_x16",  {8<<10,   16, { 1, 1, 4, 4, 1<<15, 1<<10}}},
      {"LPDDR5_16Gb_x16", {16<<10,  16, { 1, 1, 4, 4, 1<<16, 1<<10}}},
      {"LPDDR5_32Gb_x16", {32<<10,  16, { 1, 1, 4, 4, 1<<17, 1<<10}}},
      // {"LPDDR5_AiM_32Gb", {32<<10,  16, {1, 1, 4, 4, 1<<17, 1<<10}}},
      // 64 GB AiM
      {"16X_LPDDR5_AiM_32Gb", {32<<10, 16, {16, 1, 4, 4, 1<<17, 1<<10}}}
    };

    inline static const std::map<std::string, std::vector<int>> timing_presets = {
      //   name         rate   nBL16  nCL  nRCD  nRPab  nRPpb  nRAS  nRC  nWR  nRTP  nCWL  nCCD  nRRD  nWTRS  nWTRL  nFAW  nPPD  nRFCab  nRFCpb nREFI nPBR2PBR nPBR2ACT nCS, // AiM: nRCDRDMAC  nRCDRDAF  nRCDRDCP  nRCDWRCP  nRCDEWMUL  nCLREG  nCLGB  nCWLREG  nCWLGB  nWPRE  tCK_ps  
      {"LPDDR5_6400",  {6400,    4,   20,   15,   17,    15,    34,   30,  28,   4,   11,    4,    4,    5,    10,    16,    2,    -1,     -1,    -1,    -1,      -1,    2,             0,         0,        0,        0,        0,         0,     0,      0,      0,      0,    1250}},
      // AiM
      {"LPDDR5_AiM",   {6400,    4,   20,   15,   17,    15,    34,   30,  28,   4,   11,    4,    4,    5,    10,    16,    2,    -1,     -1,    -1,    -1,      -1,    2,             56,        86,       66,       48,       25,        0,     1,      1,      1,      1,    1250}},
    };


  /************************************************
   *                Organization
   ***********************************************/   
    const int m_internal_prefetch_size = 16;

    inline static constexpr ImplDef m_levels = {
      "channel", "rank", "bankgroup", "bank", "row", "column",    
    };


  /************************************************
   *             Requests & Commands
   ***********************************************/
    inline static constexpr ImplDef m_commands = {
      "ACT-1",  "ACT-2",
      "PRE",    "PREA",
      // WCK2CK Sync
      "CASRD",  "CASWR",
      "RD16",   "WR16",   "RD16A",   "WR16A",
      "REFab",  "REFpb",  "REFab_end",
      "RFMab",  "RFMpb",
      // Single-bank AiM commands leverage the conventional RD, WR, etc.
      "MACSB", "AFSB", "RDCP", "WRCP",
      // Multi-bank AiM commands
      "ACT4_BG-1", "ACT4_BG-2", "PRE4_BG",
      "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD",
      // TODO: 4-bank commands for inter-bank group
      // "ACT4_BKS-1", "ACTAF4_BKS-1", "ACT4_BKS-1", "ACTAF4_BKS-1", "PRE4_BKS",
      // "CASRD4B_INTER", "CASWR4B_INTER",
      // "MAC4B_INTER", "AF4B_INTER",
      "ACT16-1", "ACT16-2",
      "MACAB", "AFAB", "WRAFLUT", "WRBK",
      // No-bank AiM commands
      "WRGB", "WRMAC", "WRBIAS", "RDMAC", "RDAF",
    };

    inline static const ImplLUT m_command_addressing_level = LUT (
      m_commands, m_levels, {
        {"ACT-1", "row"},    {"ACT-2",  "row"},
        {"PRE",   "bank"},   {"PREA",   "rank"},
        {"CASRD", "rank"},   {"CASWR",  "rank"},
        {"RD16",  "column"}, {"WR16",   "column"}, {"RD16A", "column"}, {"WR16A", "column"},
        {"REFab", "rank"},   {"REFpb",  "rank"},   {"REFab_end", "rank"},
        {"RFMab", "rank"},   {"RFMpb",  "rank"},
        // Single-bank AiM commands
        {"MACSB", "column"}, {"AFSB", "column"}, {"RDCP", "column"}, {"WRCP", "column"},
        // Multi-bank AiM commands
        {"ACT4_BG-1", "row"}, {"ACT4_BG-2", "row"}, {"PRE4_BG", "bank"},
        {"MAC4B_INTRA", "column"}, {"AF4B_INTRA", "column"}, {"EWMUL", "column"}, {"EWADD", "column"},
        // TODO: 4-bank commands for inter-bank group
        // {"ACT4_BKS-1", "row"}, {"ACTAF4_BKS-1", "row"}, {"ACT4_BKS-2", "row"}, {"ACTAF4_BKS-2", "row"}, {"PRE4_BKS", "bank"},
        // {"CASRD4B_INTER", "rank"}, {"CASWR4B_INTER",  "rank"},
        // {"MAC4B_INTER", "column"}, {"AF4B_INTER", "column"},
        {"ACT16-1", "row"},  {"ACT16-2", "row"},
        {"MACAB", "column"}, {"AFAB", "column"}, {"WRAFLUT", "row"},  {"WRBK", "column"},
        // No-bank AiM commands
        {"WRGB", "channel"}, {"WRMAC", "bank"}, {"WRBIAS", "bank"},
        {"RDMAC", "bank"}, {"RDAF", "bank"},
      }
    );

    inline static const ImplLUT m_command_action_scope = LUT (
      m_commands, m_levels, {
        {"ACT-1", "row"},    {"ACT-2",  "row"},
        {"PRE",   "bank"},   {"PREA",   "rank"},
        {"CASRD", "rank"},   {"CASWR",  "rank"},
        {"RD16",  "column"}, {"WR16",   "column"}, {"RD16A", "column"}, {"WR16A", "column"},
        {"REFab", "rank"},   {"REFpb",  "rank"},   {"REFab_end", "rank"},
        {"RFMab", "rank"},   {"RFMpb",  "rank"},
        // Single-bank AiM commands
        {"MACSB", "column"}, {"AFSB", "column"}, {"RDCP", "column"}, {"WRCP", "column"},
        // 4-bank commands for intra-bank group
        {"ACT4_BG-1", "bankgroup"}, {"ACT4_BG-2", "bankgroup"}, {"PRE4_BG", "bankgroup"},
        {"MAC4B_INTRA", "bankgroup"}, {"AF4B_INTRA", "bankgroup"}, {"EWMUL", "bankgroup"}, {"EWADD", "bankgroup"},
        // TODO: 4-bank commands for inter-bank group
        // {"ACT4_BKS-1", "bank"}, {"ACTAF4_BKS-1", "bank"}, {"ACT4_BKS-2", "bank"}, {"ACTAF4_BKS-2", "bank"}, {"PRE4_BKS", "bank"},
        // {"CASRD4B_INTER", "rank"}, {"CASWR4B_INTER",  "rank"},
        // {"MAC4B_INTER", "bank"}, {"AF4B_INTER", "bank"}, {"EWMUL", "bank"}, {"EWADD", "bank"},
        // all-bank commands
        {"ACT16-1", "rank"}, {"ACT16-2", "rank"},
        {"MACAB", "rank"}, {"AFAB", "rank"}, {"WRAFLUT", "rank"}, {"WRBK", "rank"},
        // No-bank AiM commands
        {"WRGB", "rank"}, {"WRMAC", "rank"}, {"WRBIAS", "rank"},
        {"RDMAC", "rank"}, {"RDAF", "rank"},
      }
    );

    inline static const ImplLUT m_command_meta = LUT<DRAMCommandMeta> (
      m_commands, {
                // open?   close?   access?  refresh?
        {"ACT-1", {false,  false,   false,   false}},
        {"ACT-2", {true,   false,   false,   false}},
        {"PRE",   {false,  true,    false,   false}},
        {"PREA",  {false,  true,    false,   false}},
        {"CASRD", {false,  false,   false,   false}},
        {"CASWR", {false,  false,   false,   false}},
        {"RD16",  {false,  false,   true,    false}},
        {"WR16",  {false,  false,   true,    false}},
        {"RD16A", {false,  true,    true,    false}},
        {"WR16A", {false,  true,    true,    false}},
        {"REFab", {false,  false,   false,   true }},
        {"REFab_end", {false,  true,    false,   false}},
        {"REFpb", {false,  false,   false,   true }},
        {"RFMab", {false,  false,   false,   true }},
        {"RFMpb", {false,  false,   false,   true }},
        // Single-bank AiM commands
        {"MACSB", {false,  false,   true,    false}},
        {"AFSB",  {false,  false,   true,    false}},
        {"RDCP",  {false,  false,   true,    false}},
        {"WRCP",  {false,  false,   true,    false}},
        // Multi-bank AiM commands
        {"ACT4_BG-1",     {false,   false,   false,   false}},
        {"ACT4_BG-2",     {true,   false,   false,   false}},
        {"PRE4_BG",       {false,  true,    false,   false}},
        {"MAC4B_INTRA",   {false,  false,   true,    false}},
        {"AF4B_INTRA" ,   {false,  false,   true,    false}},
        {"EWMUL",         {false,  false,   true,    false}},
        {"EWADD",         {false,  false,   true,    false}},
        // TODO: 4-bank commands for inter-bank group
        // {"ACT4_BKS" ,   {true,  false,    false,   false}},
        // {"ACTAF4_BKS" ,   {true,  false,    false,   false}},
        // {"PRE4_BKS" ,   {false,  true,    false,   false}},
        // {"MAC4B_INTER" ,   {false,  false,    true,   false}},
        // {"AF4B_INTER" ,   {false,  false,    true,   false}},
        {"ACT16-1",   {false,   false,   false,   false}},
        {"ACT16-2",   {true,   false,   false,   false}},
        {"MACAB",   {false,  false,   true,    false}},
        {"AFAB" ,   {false,  false,   true,    false}},
        {"WRAFLUT", {false,  true,    true,    false}},
        {"WRBK",    {false,  false,   true,    false}},
        // AiM DMA commands
        {"WRGB",    {false,  false,   false,   false}},
        {"WRMAC",   {false,  false,   false,   false}},
        {"WRBIAS",  {false,  false,   false,   false}},
        {"RDMAC",   {false,  false,   false,   false}},
        {"RDAF",    {false,  false,   false,   false}},
      }
    );

    inline static constexpr ImplDef m_requests = {
      "read16", "write16",
      "all-bank-refresh", "per-bank-refresh",
      "open-row", "close-row"
    };

    inline static const ImplLUT m_request_translations = LUT (
      m_requests, m_commands, {
        {"read16", "RD16"}, {"write16", "WR16"},
        {"all-bank-refresh", "REFab"}, {"per-bank-refresh", "REFpb"},
        {"open-row", "ACT-2"}, {"close-row", "PRE"}
      }
    );

    inline static constexpr ImplDef m_aim_req = {
      // Single-bank AiM commands
      "MAC_SBK", "AF_SBK", "COPY_BKGB", "COPY_GBBK",
      // Multi-bank AiM commands
      "MAC_4BK_INTRA_BG", "AF_4BK_INTRA_BG", "EWMUL", "EWADD",
      // TODO: 4-bank commands for inter-bank group
      // "MAC_4BK_INTER_BG", "AF_4BK_INTER_BG",
      "MAC_ABK", "AF_ABK", "WR_AFLUT", "WR_BK",
      // AiM DMA commands
      "WR_GB", "WR_MAC", "WR_BIAS", "RD_MAC", "RD_AF",
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
        {"WR_GB", "WRGB"}, {"WR_MAC", "WRMAC"}, {"WR_BIAS", "WRBIAS"}, {"RD_MAC", "RDMAC"}, {"RD_AF", "RDAF"},
      }
    );
   
  /************************************************
   *                   Timing
   ***********************************************/
    inline static constexpr ImplDef m_timings = {
      "rate", 
      "nBL16", "nCL", "nRCD", "nRPab", "nRPpb", "nRAS", "nRC", "nWR", "nRTP", "nCWL",
      "nCCD",
      "nRRD",
      "nWTRS", "nWTRL",
      "nFAW",
      "nPPD",
      "nRFCab", "nRFCpb", "nREFI",
      "nPBR2PBR", "nPBR2ACT",
      "nCS",
      // AiM
      "nRCDRDMAC", "nRCDRDAF", "nRCDRDCP", "nRCDWRCP", "nRCDEWMUL",
      "nCLGB", "nCLREG", "nCWLGB", "nCWLREG", "nWPRE",
      "tCK_ps",
    };


  /************************************************
   *                 Node States
   ***********************************************/
    inline static constexpr ImplDef m_states = {
    //    ACT-1       ACT-2
       "Pre-Opened", "Opened", "Closed", "PowerUp", "N/A", "Refreshing"
    };

    inline static const ImplLUT m_init_states = LUT (
      m_levels, m_states, {
        {"channel",   "N/A"}, 
        {"rank",      "PowerUp"},
        {"bankgroup", "N/A"},
        {"bank",      "Closed"},
        {"row",       "Closed"},
        {"column",    "N/A"},
      }
    );

  public:
    struct Node : public DRAMNodeBase<LPDDR5> {
      Clk_t m_final_synced_cycle = -1; // Extra CAS Sync command needed for RD/WR after this cycle
      Node(LPDDR5* dram, Node* parent, int level, int id) : DRAMNodeBase<LPDDR5>(dram, parent, level, id) {};
    };
    std::vector<Node*> m_channels;
    
    FuncMatrix<ActionFunc_t<Node>>  m_actions;
    FuncMatrix<PreqFunc_t<Node>>    m_preqs;
    FuncMatrix<RowhitFunc_t<Node>>  m_rowhits;
    FuncMatrix<RowopenFunc_t<Node>> m_rowopens;

  public:
    void tick() override {
      m_clk++;

      if (m_future_actions.size() > 0) {
        std::cerr << "[AiM_LPDDR5::tick] Current cycle: " << m_clk
                  << ", Future actions pending: " << m_future_actions.size() << std::endl;
      }

      // Process future actions (e.g., REFab_end after nRFCab cycles)
      for (int i = m_future_actions.size() - 1; i >= 0; i--) {
        auto& future_action = m_future_actions[i];
        if (future_action.clk == m_clk) {
          int channel_id = future_action.addr_h[m_levels["channel"]];
          int rank_id = future_action.addr_h[m_levels["rank"]];
          // std::cerr << "[AiM_LPDDR5::tick] EXECUTING future action: cmd=" << m_commands(future_action.cmd)
          //           << " channel=" << channel_id
          //           << " rank=" << rank_id
          //           << " cycle=" << m_clk << std::endl;

          // Log bank states before REFab_end
          if (future_action.cmd == m_commands["REFab_end"]) {
            auto* rank = m_channels[channel_id]->m_child_nodes[rank_id];
            std::cerr << "[AiM_LPDDR5::tick] REFab_end - Bank states BEFORE:" << std::endl;
            for (auto* bg : rank->m_child_nodes) {
              for (auto* bank : bg->m_child_nodes) {
                std::cerr << "  BG[" << bg->m_node_id << "] Bank[" << bank->m_node_id << "] = "
                         << m_states(bank->m_state) << std::endl;
              }
            }
          }

          m_channels[channel_id]->update_states(future_action.cmd, future_action.addr_h, m_clk);

          // Log bank states after REFab_end
          if (future_action.cmd == m_commands["REFab_end"]) {
            auto* rank = m_channels[channel_id]->m_child_nodes[rank_id];
            std::cerr << "[AiM_LPDDR5::tick] REFab_end - Bank states AFTER:" << std::endl;
            for (auto* bg : rank->m_child_nodes) {
              for (auto* bank : bg->m_child_nodes) {
                std::cerr << "  BG[" << bg->m_node_id << "] Bank[" << bank->m_node_id << "] = "
                         << m_states(bank->m_state) << std::endl;
              }
            }
          }

          m_future_actions.erase(m_future_actions.begin() + i);
          std::cerr << "[AiM_LPDDR5::tick] Future action executed and removed. Remaining: "
                    << m_future_actions.size() << std::endl;
        }
      }
    };

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
      int bankgroup_id = addr_h[m_levels["bankgroup"]];
      int bank_id = addr_h[m_levels["bank"]];
      switch (command)
      {
      case m_commands["PREA"]: {
        m_open_rows[channel_id] = 0;
        break;
      }
      case m_commands["PRE4_BG"]: {
        for (int bank_id_in_bg = bankgroup_id * 4; bank_id_in_bg < (bankgroup_id + 1) * 4; bank_id_in_bg++) {
          m_open_rows[channel_id] &= ~((uint16_t)(1 << bank_id_in_bg));
        }
        break;
      }
      case m_commands["PRE"]:
      case m_commands["RD16A"]:
      case m_commands["WR16A"]: {
        m_open_rows[channel_id] &= ~((uint16_t)(1 << bank_id));
        break;
      }
      case m_commands["ACT16-2"]: {
        m_open_rows[channel_id] = (uint16_t)(0xFFFF);
        break;
      }
      case m_commands["ACT4_BG-2"]: {
        for (int bank_id_in_bg = bankgroup_id * 4; bank_id_in_bg < (bankgroup_id + 1) * 4; bank_id_in_bg++) {
          m_open_rows[channel_id] |= ((uint16_t)(1 << bank_id_in_bg));
        }
        break;
      }
      case m_commands["ACT-2"]: {
        m_open_rows[channel_id] |= (uint16_t)(1 << bank_id);
        break;
      }
      case m_commands["REFab"]: {
        // Schedule REFab_end to execute after nRFCab cycles
        Clk_t refab_end_cycle = m_clk + m_timing_vals("nRFCab") - 1;
        std::cerr << "[AiM_LPDDR5::issue_command] REFab issued at cycle " << m_clk
                  << ". Scheduling REFab_end for cycle " << refab_end_cycle
                  << " (nRFCab=" << m_timing_vals("nRFCab") << ")"
                  << " channel=" << addr_h[m_levels["channel"]]
                  << " rank=" << addr_h[m_levels["rank"]] << std::endl;
        m_future_actions.push_back({m_commands["REFab_end"], addr_h, m_clk + m_timing_vals("nRFCab") - 1});
        std::cerr << "[AiM_LPDDR5::issue_command] Future actions count after REFab: "
                  << m_future_actions.size() << std::endl;
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
      m_channel_width = param_group("org").param<int>("channel_width").default_val(32);

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
      size_t _density = size_t(m_organization.count[m_levels["rank"]]) *
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
          case 16: return 0;
          default: return -1;
        }
      }(m_organization.dq);

      int rate_id = [](int rate) -> int {
        switch (rate) {
          case 6400:  return 0;
          default:    return -1;
        }
      }(m_timing_vals("rate"));


      // Refresh timings
      // tRFC table (unit is nanosecond!)
      // AiM (32Gb) defines arbitrary values.
      constexpr int tRFCab_TABLE[5] = {
      //  2Gb   4Gb   8Gb  16Gb  32Gb
          130,  180,  210,  280,  330
      };

      constexpr int tRFCpb_TABLE[5] = {
      //  2Gb   4Gb   8Gb  16Gb  32Gb
          60,   90,   120,  140,  160
      };

      constexpr int tPBR2PBR_TABLE[5] = {
      //  2Gb   4Gb   8Gb  16Gb  32Gb
          60,   90,   90,  90,    90
      };

      constexpr int tPBR2ACT_TABLE[5] = {
      //  2Gb   4Gb   8Gb  16Gb  32Gb
          8,    8,    8,   8,    8
      };

      // tREFI(base) table (unit is nanosecond!)
      constexpr int tREFI_BASE = 3906;
      int density_id = [](int density_Mb) -> int { 
        switch (density_Mb) {
          case 2048:  return 0;
          case 4096:  return 1;
          case 8192:  return 2;
          case 16384: return 3;
          case 32768: return 4;
          default:    return -1;
        }
      }(m_organization.density);

      m_timing_vals("nRFCab")    = JEDEC_rounding(tRFCab_TABLE[density_id], tCK_ps);
      m_timing_vals("nRFCpb")    = JEDEC_rounding(tRFCpb_TABLE[density_id], tCK_ps);
      m_timing_vals("nPBR2PBR")  = JEDEC_rounding(tPBR2PBR_TABLE[density_id], tCK_ps);
      m_timing_vals("nPBR2ACT")  = JEDEC_rounding(tPBR2ACT_TABLE[density_id], tCK_ps);
      m_timing_vals("nREFI")     = JEDEC_rounding(tREFI_BASE, tCK_ps);

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
      /********************************************************************************
       * Set execution time (completion latency) for the commands for pending
       *********************************************************************************/
      // Conventional RD/WR commands
      m_command_latencies("RD16") = m_timing_vals("nCL") + m_timing_vals("nBL16");
      m_command_latencies("WR16") = m_timing_vals("nCWL") + m_timing_vals("nBL16");
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
      m_command_latencies("WRGB")  = m_timing_vals("nCWLGB")  + m_timing_vals("nBL16");
      m_command_latencies("WRMAC") = m_timing_vals("nCWLREG") + m_timing_vals("nBL16");
      m_command_latencies("WRBIAS") = m_timing_vals("nCWLREG") + m_timing_vals("nBL16");
      m_command_latencies("RDMAC") = m_timing_vals("nCLREG")  + m_timing_vals("nBL16");
      m_command_latencies("RDAF")  = m_timing_vals("nCLREG")  + m_timing_vals("nBL16");
      // AiM timings
      m_timing_vals("nCLREG")  = 0;
      m_timing_vals("nCLGB")   = 1;
      m_timing_vals("nCWLREG") = 1;
      m_timing_vals("nCWLGB")  = 1;
      m_timing_vals("nWPRE")   = 1;

      // Populate the timing constraints
      #define V(timing) (m_timing_vals(timing))
      populate_timingcons(this, {
        /***************************************************************************************************
         *                                            Channel
         ***************************************************************************************************/
        /************************************************************
         * CAS <-> CAS
         * Data bus (DQ) occupancy
         *************************************************************/
        {.level = "channel", .preceding = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .latency = V("nBL16")},
        {.level = "channel", .preceding = {"WR16", "WR16A", "WRAFLUT", "WRBK"}, .following = {"WR16", "WR16A", "WRAFLUT", "WRBK"}, .latency = V("nBL16")},
        /***************************************************************************************************
         *                                     DIFFERENT BankGroup (Rank)
         ***************************************************************************************************/
        /************************************************************
         * RAS <-> RAS
         *************************************************************/
        // ACT <-> ACT
        {.level = "rank", .preceding = {"ACT16-1"}, .following = {"ACT16-1"}, .latency = V("nRRD")},
        {.level = "rank", .preceding = {"ACT4_BG-1"}, .following = {"ACT-1", "ACT4_BG-1"}, .latency = V("nRRD")},
        {.level = "rank", .preceding = {"ACT-1"}, .following = {"ACT-1", "ACT4_BG-1"}, .latency = V("nRRD")},
        {.level = "rank", .preceding = {"ACT-1"}, .following = {"ACT-1"}, .latency = V("nFAW"), .window = 4},
        // ACT <-> PRE
        {.level = "rank", .preceding = {"ACT-1", "ACT4_BG-1", "ACT16-1"}, .following = {"PREA"}, .latency = V("nRAS")},
        {.level = "rank", .preceding = {"PREA"},  .following = {"ACT-1", "ACT4_BG-1", "ACT16-1"}, .latency = V("nRPab")},
        {.level = "rank", .preceding = {"PRE4_BG"}, .following = {"ACT16-1"}, .latency = V("nRPab")},
        {.level = "rank", .preceding = {"PRE"}, .following = {"ACT16-1"}, .latency = V("nRPpb")},
        /************************************************************
         * CAS <-> RAS
         *************************************************************/
        {.level = "rank", .preceding = {"ACT-1", "ACT4_BG-1", "ACT16-1"}, .following = {"MACAB", "AFAB", "WRAFLUT", "WRBK"}, .latency = V("nRCD")},
        {.level = "rank", .preceding = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"PREA"}, .latency = V("nRTP")},
        {.level = "rank", .preceding = {"WR16", "WR16A", "WRAFLUT", "WRBK"}, .following = {"PREA"}, .latency = V("nCWL")+V("nBL16")+V("nWR")},
        /************************************************************
         * CAS <-> CAS
         *************************************************************/
        // RD -> RD, WR -> WR
        {.level = "rank", .preceding = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .latency = V("nCCD")},
        {.level = "rank", .preceding = {"WR16", "WR16A", "WRAFLUT", "WRBK"}, .following = {"WR16", "WR16A", "WRAFLUT", "WRBK"}, .latency = V("nCCD")},
        // RD -> WR; Minimum Read to Write, Assuming tWPRE = 1 tCK                          
        {.level = "rank", .preceding = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"WR16", "WR16A", "WRAFLUT", "WRBK"}, .latency = V("nCL")+V("nBL16")+2-V("nCWL")},      
        // WR -> RD; Minimum Read after Write
        {.level = "rank", .preceding = {"WR16", "WR16A", "WRAFLUT", "WRBK"}, .following = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .latency = V("nCWL")+V("nBL16")+V("nWTRS")},
        /************************************************************
         * CAS <-> CAS between sibling ranks, nCS (rank switching) is needed for new DQS
         *************************************************************/
        // RD -> RD/WR
        {.level = "rank", .preceding = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB", "WR16", "WR16A", "WRAFLUT", "WRBK"}, .latency = V("nBL16")+V("nCS"), .is_sibling = true},
        // WR -> RD
        {.level = "rank", .preceding = {"WR16", "WR16A", "WRAFLUT", "WRBK"}, .following = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .latency = V("nCL")+V("nBL16")+V("nCS")-V("nCWL"), .is_sibling = true},
        // WR -> WR
        {.level = "rank", .preceding = {"WR16", "WR16A", "WRAFLUT", "WRBK"}, .following = {"WR16", "WR16A", "WRAFLUT", "WRBK"}, .latency = V("nBL16")+V("nCS"), .is_sibling = true},
        /************************************************************ 
         * REF
         *************************************************************/
        /// RAS <-> REF
        {.level = "rank", .preceding = {"ACT-1", "ACT4_BG-1", "ACT4_BG-2", "ACT16-1", "ACT16-2"}, .following = {"REFab"}, .latency = V("nRC")},
        {.level = "rank", .preceding = {"PRE", "PRE4_BG"},   .following = {"REFab"}, .latency = V("nRPpb")},
        {.level = "rank", .preceding = {"PREA"},  .following = {"REFab"}, .latency = V("nRPab")},
        {.level = "rank", .preceding = {"RD16A"}, .following = {"REFab"}, .latency = V("nRPpb")+V("nRTP")},
        {.level = "rank", .preceding = {"WR16A"}, .following = {"REFab"}, .latency = V("nCWL")+V("nBL16")+V("nWR")+V("nRPpb")},
        {.level = "rank", .preceding = {"REFab"}, .following = {"REFab", "ACT-1", "ACT4_BG-1", "ACT4_BG-2", "ACT16-1", "ACT16-2", "PRE", "PRE4_BG", "PREA", "REFpb"}, .latency = V("nRFCab")},
        {.level = "rank", .preceding = {"ACT-1"}, .following = {"REFpb"}, .latency = V("nPBR2ACT")},
        {.level = "rank", .preceding = {"REFpb"}, .following = {"REFpb"}, .latency = V("nPBR2PBR")},
        /***************************************************************************************************
         *                                     SAME BankGroup (Rank)
         ***************************************************************************************************/
        /************************************************************ 
         * RAS <-> RAS
         *************************************************************/
        {.level = "bankgroup", .preceding = {"ACT-1"}, .following = {"ACT-1"}, .latency = V("nRRD")},
        {.level = "bankgroup", .preceding = {"ACT-1", "ACT4_BG-1", "ACT16-1"}, .following = {"PRE4_BG"}, .latency = V("nRAS")},
        {.level = "bankgroup", .preceding = {"PRE4_BG"}, .following = {"ACT-1", "ACT4_BG-1"}, .latency = V("nRPab")},
        {.level = "bankgroup", .preceding = {"PRE"}, .following = {"ACT4_BG-1"}, .latency = V("nRPpb")},
        /************************************************************
         * CAS <-> RAS
         *************************************************************/
        {.level = "bankgroup", .preceding = {"ACT4_BG-1", "ACT16-1"}, .following = {"MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD"}, .latency = V("nRCD")},
        {.level = "bankgroup", .preceding = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"PRE4_BG"}, .latency = V("nRTP")},
        {.level = "bankgroup", .preceding = {"WRAFLUT", "WRBK"}, .following = {"PRE4_BG"}, .latency = V("nCWL")+V("nBL16")+V("nWR")},
        /************************************************************
         * CAS <-> CAS
         *************************************************************/
        {.level = "bankgroup", .preceding = {"RD16", "RD16A"}, .following = {"RD16", "RD16A"}, .latency = V("nCCD")},
        {.level = "bankgroup", .preceding = {"WR16", "WR16A"}, .following = {"WR16", "WR16A"}, .latency = V("nCCD")},
        {.level = "bankgroup", .preceding = {"WR16", "WR16A"}, .following = {"RD16", "RD16A", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD"}, .latency = V("nCWL")+V("nBL16")+V("nWTRL")},
        /***************************************************************************************************
         *                                             Bank
         ***************************************************************************************************/
        /************************************************************
         * RAS <-> RAS
         *************************************************************/
        {.level = "bank", .preceding = {"ACT-1"}, .following = {"ACT-1"}, .latency = V("nRC")},
        {.level = "bank", .preceding = {"ACT-1", "ACT4_BG-1", "ACT16-1"}, .following = {"PRE"}, .latency = V("nRAS")},
        {.level = "bank", .preceding = {"PRE"},   .following = {"ACT-1"}, .latency = V("nRPpb")},
        /************************************************************
         * CAS <-> RAS
         *************************************************************/
        {.level = "bank", .preceding = {"ACT-1", "ACT4_BG-1", "ACT16-1"}, .following = {"RD16", "RD16A", "WR16", "WR16A"}, .latency = V("nRCD")},
        {.level = "bank", .preceding = {"RD16", "MAC4B_INTRA", "AF4B_INTRA", "EWMUL", "EWADD", "MACAB", "AFAB"}, .following = {"PRE"}, .latency = V("nRTP")},
        {.level = "bank", .preceding = {"WR16", "WRAFLUT", "WRBK"},  .following = {"PRE"}, .latency = V("nCWL")+V("nBL16")+V("nWR")},
        {.level = "bank", .preceding = {"RD16A"}, .following = {"ACT-1"}, .latency = V("nRTP")+V("nRPpb")},
        {.level = "bank", .preceding = {"WR16A"}, .following = {"ACT-1"}, .latency = V("nCWL")+V("nBL16")+V("nWR")+V("nRPpb")},
        });
      #undef V
    };

    void set_actions() {
      m_actions.resize(m_levels.size(), std::vector<ActionFunc_t<Node>>(m_commands.size()));

      #define ACTION_DEF(OP)                                                                                              \
      m_actions[m_levels["rank"]][m_commands[#OP]] = [this] (Node *node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) { \
        node->m_final_synced_cycle = clk + m_command_latencies(#OP);                                                      \
      };
      
      // m_final_synced_cycle is exactly the execution time for the command.
      // Rank Actions
      m_actions[m_levels["rank"]][m_commands["PREA"]] = Lambdas::Action::Rank::PREab<LPDDR5>;
      m_actions[m_levels["rank"]][m_commands["REFab"]] = Lambdas::Action::Rank::REFab<LPDDR5>;
      m_actions[m_levels["rank"]][m_commands["REFab_end"]] = Lambdas::Action::Rank::REFab_end<LPDDR5>;
      m_actions[m_levels["rank"]][m_commands["CASRD"]] = [] (Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
        node->m_final_synced_cycle = clk + m_timings["nCL"] + m_timings["nBL16"] + 1;
      };
      m_actions[m_levels["rank"]][m_commands["CASWR"]] = [] (Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
        node->m_final_synced_cycle = clk + m_timings["nCWL"] + m_timings["nBL16"] + 1;
      };
      ACTION_DEF(RD16);
      ACTION_DEF(WR16);
      ACTION_DEF(MACSB);
      ACTION_DEF(AFSB);
      ACTION_DEF(RDCP);
      ACTION_DEF(WRCP);

      // Rank actions; AiM
      m_actions[m_levels["rank"]][m_commands["ACT16-1"]] = [] (Node* node, int cmd, const AddrHierarchy_t &addr_h, Clk_t clk) {
        for (auto bg : node->m_child_nodes) {
          for (auto bank : bg->m_child_nodes) {
            bank->m_state = m_states["Pre-Opened"];
            bank->m_row_state[addr_h[m_levels["row"]]] = m_states["Pre-Opened"];
          }
        }
      };
      m_actions[m_levels["rank"]][m_commands["ACT16-2"]]   = Lambdas::Action::Rank::ACTab<LPDDR5>;
      ACTION_DEF(MACAB);
      ACTION_DEF(AFAB);
      ACTION_DEF(WRAFLUT);
      ACTION_DEF(WRBK);

      ACTION_DEF(MAC4B_INTRA);
      ACTION_DEF(AF4B_INTRA);
      ACTION_DEF(EWMUL);
      ACTION_DEF(EWADD);

      // Bank group Actions; AiM
      // Action for ACT_BG-1: Transition all banks in the group to "Pre-Opened".
      m_actions[m_levels["bankgroup"]][m_commands["ACT4_BG-1"]] = [this] (Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
        int row_id = addr_h[m_levels["row"]];
        for (auto bank : node->m_child_nodes) {
          bank->m_state = m_states["Pre-Opened"];
          bank->m_row_state[row_id] = m_states["Pre-Opened"];
        }
      };
      
      // Action for ACT_BG-2: Transition all banks in the group to "Opened".
      m_actions[m_levels["bankgroup"]][m_commands["ACT4_BG-2"]]   = Lambdas::Action::BankGroup::ACT4b_bg<LPDDR5>;
      m_actions[m_levels["bankgroup"]][m_commands["PRE4_BG"]]     = Lambdas::Action::BankGroup::PRE4b_bg<LPDDR5>;

      // Bank actions
      m_actions[m_levels["bank"]][m_commands["ACT-1"]] = [] (Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
        int row_id = addr_h[m_levels["row"]];
        node->m_state = m_states["Pre-Opened"];
        node->m_row_state[row_id] = m_states["Pre-Opened"];
      };
      m_actions[m_levels["bank"]][m_commands["ACT-2"]] = Lambdas::Action::Bank::ACT<LPDDR5>;
      m_actions[m_levels["bank"]][m_commands["PRE"]]   = Lambdas::Action::Bank::PRE<LPDDR5>;
      m_actions[m_levels["bank"]][m_commands["RD16A"]] = Lambdas::Action::Bank::PRE<LPDDR5>;
      m_actions[m_levels["bank"]][m_commands["WR16A"]] = Lambdas::Action::Bank::PRE<LPDDR5>;
    };

    void set_preqs() {
      m_preqs.resize(m_levels.size(), std::vector<PreqFunc_t<Node>>(m_commands.size()));

      // Rank Preqs
      m_preqs[m_levels["rank"]][m_commands["REFab"]] = Lambdas::Preq::Rank::RequireAllBanksClosed<LPDDR5>;
      m_preqs[m_levels["rank"]][m_commands["RFMab"]] = Lambdas::Preq::Rank::RequireAllBanksClosed<LPDDR5>;

      m_preqs[m_levels["rank"]][m_commands["REFpb"]] = [this] (Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) {
        for (auto bg : node->m_child_nodes) {
          for (auto bank : bg->m_child_nodes) {
            int num_banks_per_bg = m_organization.count[m_levels["bank"]];
            int flat_bankid = bank->m_node_id + bg->m_node_id * num_banks_per_bg;
            if (flat_bankid == addr_h[LPDDR5::m_levels["bank"]] || flat_bankid == addr_h[LPDDR5::m_levels["bank"]] + 8) {
              switch (node->m_state) {
                case m_states["Pre-Opened"]: return m_commands["PRE"];
                case m_states["Opened"]: return m_commands["PRE"];
              }
            }
          }
        }
        return cmd;
      };
      
      m_preqs[m_levels["rank"]][m_commands["RFMpb"]] = m_preqs[m_levels["rank"]][m_commands["REFpb"]];

      // Rank Preqs; AiM
      m_preqs[m_levels["rank"]][m_commands["MACAB"]]   = Lambdas::Preq::Rank::RequireAllRowsOpen<LPDDR5>;
      m_preqs[m_levels["rank"]][m_commands["AFAB"]]    = Lambdas::Preq::Rank::RequireAllRowsOpen<LPDDR5>;
      m_preqs[m_levels["rank"]][m_commands["WRAFLUT"]] = Lambdas::Preq::Rank::RequireAllRowsOpen<LPDDR5>;
      m_preqs[m_levels["rank"]][m_commands["WRBK"]]    = Lambdas::Preq::Rank::RequireAllRowsOpen<LPDDR5>;

      // BankGroup Preqs
      auto bankgroup_preqs = [this] (Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) -> int {
        int num_banks_in_bg = 0;
        int num_closed_banks = 0;
        int num_preopen_banks = 0;
        int num_open_banks = 0;

        for (auto bank : node->m_child_nodes) {
          num_banks_in_bg++;
          switch (bank->m_state) {
            case m_states["Opened"]:
              if (bank->m_row_state.find(addr_h[LPDDR5::m_levels["row"]]) != bank->m_row_state.end()) {
                num_open_banks++;
              } else {
                // CONFLICT; Wrong wro is fully open.
                return m_commands["PRE4_BG"];
              }
              break;
            case m_states["Pre-Opened"]:
              if (bank->m_row_state.find(addr_h[LPDDR5::m_levels["row"]]) != bank->m_row_state.end()) {
                num_preopen_banks++;
              } else {
                // CONFLICT; Wrong row is pre-open.
                return m_commands["PRE4_BG"];
              }
              break;
            case m_states["Closed"]:
              num_closed_banks++;
              break;
            case m_states["Refreshing"]:
              // Banks are refreshing - return the same command to indicate "not ready yet"
              // Timing constraints (REFab -> ACT4_BG with latency nRFCab) will enforce the wait
              // Once REFab_end executes and banks return to "Closed", this will resolve to ACT4_BG
              return cmd;
            default: return m_commands["PRE4_BG"];
          }
        }

        if (num_banks_in_bg == 0) { return cmd; }

        if (num_open_banks == num_banks_in_bg) {
          Node* rank = node->m_parent_node;
          if (rank->m_final_synced_cycle < clk) { return m_commands["CASRD"]; }
          else { return cmd; }
        }
        else if (num_preopen_banks == num_banks_in_bg) { return m_commands["ACT4_BG-2"]; }
        else if (num_closed_banks == num_banks_in_bg) { return m_commands["ACT4_BG-1"]; }
        // Mixed state
        else { return m_commands["PRE4_BG"]; }
      };
      m_preqs[m_levels["bankgroup"]][m_commands["MAC4B_INTRA"]] = bankgroup_preqs;
      m_preqs[m_levels["bankgroup"]][m_commands["AF4B_INTRA"]]  = bankgroup_preqs;
      m_preqs[m_levels["bankgroup"]][m_commands["EWMUL"]]       = bankgroup_preqs;
      m_preqs[m_levels["bankgroup"]][m_commands["EWADD"]]       = bankgroup_preqs;

      // Bank Preqs
      auto bank_preqs = [this] (Node* node, int cmd, const AddrHierarchy_t& addr_h, Clk_t clk) -> int {
        switch (node->m_state) {
          case m_states["Closed"]: return m_commands["ACT-1"];
          case m_states["Pre-Opened"]: return m_commands["ACT-2"];
          case m_states["Opened"]: {
            if (node->m_row_state.find(addr_h[m_levels["row"]]) != node->m_row_state.end()) {
              Node* rank = node->m_parent_node->m_parent_node;
              if (rank->m_final_synced_cycle < clk) {
                // Determine the appropriate CAS command based on the original command
                if (cmd == m_commands["WR16"] || cmd == m_commands["WRCP"]) { return m_commands["CASWR"]; }
                else { return m_commands["CASRD"]; }
              } else { return cmd; }
            } else { return m_commands["PRE"]; }
          }
          case m_states["Refreshing"]:
            // Bank is refreshing - return the same command to indicate "not ready yet"
            // Timing constraints (REFab -> ACT with latency nRFCab) will enforce the wait
            // Once REFab_end executes and banks return to "Closed", this will resolve to ACT-1
            return cmd;
          default: {
            spdlog::error("[Preq::Bank] Invalid bank state for an RD/WR command!");
            std::exit(-1);
          } 
        }
      };
      m_preqs[m_levels["bank"]][m_commands["RD16"]] = bank_preqs;
      m_preqs[m_levels["bank"]][m_commands["WR16"]] = bank_preqs;
      
      // Bank Preqs; AiM
      m_preqs[m_levels["bank"]][m_commands["MACSB"]] = bank_preqs;
      m_preqs[m_levels["bank"]][m_commands["AFSB"]]  = bank_preqs;
      m_preqs[m_levels["bank"]][m_commands["RDCP"]]  = bank_preqs;
      m_preqs[m_levels["bank"]][m_commands["WRCP"]]  = bank_preqs;
    };

    void set_rowhits() {
      m_rowhits.resize(m_levels.size(), std::vector<RowhitFunc_t<Node>>(m_commands.size()));

      m_rowhits[m_levels["bank"]][m_commands["RD16"]] = Lambdas::RowHit::Bank::RDWR<LPDDR5>;
      m_rowhits[m_levels["bank"]][m_commands["WR16"]] = Lambdas::RowHit::Bank::RDWR<LPDDR5>;

      // AiM all-bank
      m_rowhits[m_levels["channel"]][m_commands["MACAB"]]   = Lambdas::RowHit::Channel::RDWR16<LPDDR5>;
      m_rowhits[m_levels["channel"]][m_commands["AFAB"]]    = Lambdas::RowHit::Channel::RDWR16<LPDDR5>;
      m_rowhits[m_levels["channel"]][m_commands["WRAFLUT"]] = Lambdas::RowHit::Channel::RDWR16<LPDDR5>;
      m_rowhits[m_levels["channel"]][m_commands["WRBK"]]    = Lambdas::RowHit::Channel::RDWR16<LPDDR5>;

      // AiM four-bank
      m_rowhits[m_levels["bankgroup"]][m_commands["MAC4B_INTRA"]] = Lambdas::RowHit::BankGroup::RD4<LPDDR5>;
      m_rowhits[m_levels["bankgroup"]][m_commands["AF4B_INTRA"]]  = Lambdas::RowHit::BankGroup::RD4<LPDDR5>;
      m_rowhits[m_levels["bankgroup"]][m_commands["EWMUL"]]       = Lambdas::RowHit::BankGroup::RD4<LPDDR5>;
      m_rowhits[m_levels["bankgroup"]][m_commands["EWADD"]]       = Lambdas::RowHit::BankGroup::RD4<LPDDR5>;

      // AiM single bank
      m_rowhits[m_levels["bank"]][m_commands["MACSB"]] = Lambdas::RowHit::Bank::RDWR<LPDDR5>;
      m_rowhits[m_levels["bank"]][m_commands["AFSB"]]  = Lambdas::RowHit::Bank::RDWR<LPDDR5>;
      m_rowhits[m_levels["bank"]][m_commands["RDCP"]]  = Lambdas::RowHit::Bank::RDWR<LPDDR5>;
      m_rowhits[m_levels["bank"]][m_commands["AFSB"]]  = Lambdas::RowHit::Bank::RDWR<LPDDR5>;
    }

    void set_rowopens() {
      m_rowopens.resize(m_levels.size(), std::vector<RowhitFunc_t<Node>>(m_commands.size()));

      // AiM all-bank
      m_rowopens[m_levels["channel"]][m_commands["MACAB"]]   = Lambdas::RowOpen::Channel::RDWR16<LPDDR5>;
      m_rowopens[m_levels["channel"]][m_commands["AFAB"]]    = Lambdas::RowOpen::Channel::RDWR16<LPDDR5>;
      m_rowopens[m_levels["channel"]][m_commands["WRAFLUT"]] = Lambdas::RowOpen::Channel::RDWR16<LPDDR5>;
      m_rowopens[m_levels["channel"]][m_commands["WRBK"]]    = Lambdas::RowOpen::Channel::RDWR16<LPDDR5>;

      // AiM four-bank
      m_rowopens[m_levels["bankgroup"]][m_commands["MAC4B_INTRA"]] = Lambdas::RowOpen::BankGroup::RD4<LPDDR5>;
      m_rowopens[m_levels["bankgroup"]][m_commands["AF4B_INTRA"]]  = Lambdas::RowOpen::BankGroup::RD4<LPDDR5>;
      m_rowopens[m_levels["bankgroup"]][m_commands["EWMUL"]]       = Lambdas::RowOpen::BankGroup::RD4<LPDDR5>;
      m_rowopens[m_levels["bankgroup"]][m_commands["EWADD"]]       = Lambdas::RowOpen::BankGroup::RD4<LPDDR5>;

      // AiM single bank
      m_rowopens[m_levels["bank"]][m_commands["MACSB"]] = Lambdas::RowOpen::Bank::RDWR<LPDDR5>;
      m_rowopens[m_levels["bank"]][m_commands["AFSB"]]  = Lambdas::RowOpen::Bank::RDWR<LPDDR5>;
      m_rowopens[m_levels["bank"]][m_commands["RDCP"]]  = Lambdas::RowOpen::Bank::RDWR<LPDDR5>;
      m_rowopens[m_levels["bank"]][m_commands["WRCP"]]  = Lambdas::RowOpen::Bank::RDWR<LPDDR5>;
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
