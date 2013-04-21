
-- A pipelined decoder for byte-pair encoded data.
-- One byte may be read per clock cycle, as long as input is available. If the input
-- can not deliver a byte every clock cycle, external code must manage access to the
-- decoder.
-- The decoder must first be loaded with keys and pairs. Loading either resets the
-- pipeline and sends OUTPUT_VALID low..
-- Following a pipeline reset, OUPUT_VALID goes high once the pipeline is filled. This
-- takes as many clocks as there are stages.
-- INPUT_CONSUMED signals that a new INPUT value is required. Full decoding of the
-- input will require some number of padding bytes.

library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;

library UNISIM;
    use UNISIM.VComponents.all;

library work;
    use work.BP_Decoder_pkg.all;

entity BP_Decoder is
    generic(
        NSTAGES: integer
    );
    port(
        RESET:   in std_logic;-- asynchronous reset
        SYS_CLK: in std_logic;
        
        LOAD_KEY:  in  std_logic;
        LOAD_PAIR: in  std_logic;
        
        OUTPUT:       out std_logic_vector(7 downto 0);
        TAKE_OUTPUT:  in  std_logic;
        OUTPUT_VALID: out std_logic;
        
        INPUT:          in  std_logic_vector(7 downto 0);
        INPUT_CONSUMED: out std_logic
    );
end BP_Decoder;

architecture Behavioral of BP_Decoder is
    
    type pipeline_data_t is array (0 to NSTAGES) of std_logic_vector(7 downto 0);
    type byte_keys_t is array (0 to NSTAGES - 1) of std_logic_vector(7 downto 0);
    type byte_subs_t is array (0 to NSTAGES*2 - 1) of std_logic_vector(7 downto 0);
    
    signal ENABLED_STAGES: std_logic_vector(NSTAGES - 1 downto 0);
    signal PIPELINE_DATA:  pipeline_data_t;
    signal READ_EMPTY:     std_logic_vector(NSTAGES downto 0);
    signal BYTE_KEYS:      byte_keys_t;
    signal BYTE_SUBS:      byte_subs_t;
    
begin
    DecodePipeline: for I in 0 to NSTAGES - 1 generate
        BP_DecodeStage_X: BP_DecodeStage
            port map(
                RESET => RESET,
                SYS_CLK => SYS_CLK,
                
                ENABLED => ENABLED_STAGES(I),
                IN_BYTE => PIPELINE_DATA(I),
                OUT_BYTE => PIPELINE_DATA(I+1),
                
                READ => READ_EMPTY(I+1),
                EMPTY => READ_EMPTY(I),
                
                KEY => BYTE_KEYS(I),
                FIRST => BYTE_SUBS(I*2 + 1),
                SECOND => BYTE_SUBS(I*2)
            );
    end generate DecodePipeline;
    
    PIPELINE_DATA(0) <= INPUT;
    INPUT_CONSUMED <= READ_EMPTY(0) or LOAD_KEY or LOAD_PAIR;
    OUTPUT <= PIPELINE_DATA(NSTAGES);
    
    -- This process controls shifting data into the substitution key/pair arrays
    -- and managing the pipeline.
    MainProc: process(RESET, SYS_CLK) is
    begin
        if(RESET = '1') then
            -- All stages disabled by reset, enabled as they are filled
            BYTE_KEYS <= (others => (others => '0'));
            BYTE_SUBS <= (others => (others => '0'));
            ENABLED_STAGES <= (0 => '1', others => '0');
            OUTPUT_VALID <= '0';
        elsif(rising_edge(SYS_CLK)) then
            READ_EMPTY(NSTAGES) <= TAKE_OUTPUT;
            OUTPUT_VALID <= '0';
            
            if(LOAD_KEY = '1') then
                BYTE_KEYS(1 to NSTAGES - 1) <= BYTE_KEYS(0 to NSTAGES - 2);
                BYTE_KEYS(0) <= INPUT;
                ENABLED_STAGES <= (0 => '1', others => '0');
                
            elsif(LOAD_PAIR = '1') then
                BYTE_SUBS(1 to NSTAGES*2 - 1) <= BYTE_SUBS(0 to NSTAGES*2 - 2);
                BYTE_SUBS(0) <= INPUT;
                ENABLED_STAGES <= (0 => '1', others => '0');
            else
                if(ENABLED_STAGES /= (ENABLED_STAGES'range => '1')) then
                    -- Enable stages as data loads into them
                    ENABLED_STAGES(NSTAGES - 1 downto 1) <= ENABLED_STAGES(NSTAGES - 2 downto 0);
                    ENABLED_STAGES(0) <= '1';
                    READ_EMPTY(NSTAGES) <= '1';
                else
                    -- Pipeline now filled and producing data
                    OUTPUT_VALID <= '1';
                end if;
            end if;
        end if;
    end process;
end Behavioral;
