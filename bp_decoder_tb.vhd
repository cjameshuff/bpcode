
library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;

library UNISIM;
    use UNISIM.VComponents.all;

library work;
    use work.BP_Decoder_pkg.all;

entity BP_Decoder_TB is
end BP_Decoder_TB;

architecture Behavioral of BP_Decoder_TB is
    constant SYS_CLK_period: time := 31.25 ns;
    
    signal RESET: std_logic := '0';
    signal SYS_CLK: std_logic := '0';
    
    signal LOAD_KEY: std_logic := '0';
    signal LOAD_PAIR: std_logic := '0';
    
    signal OUTPUT: std_logic_vector(7 downto 0) := (others => '0');
    signal TAKE_OUTPUT: std_logic := '0';
    signal OUTPUT_VALID: std_logic := '0';
    
    signal INPUT: std_logic_vector(7 downto 0) := (others => '0');
    signal INPUT_CONSUMED: std_logic := '0';
    
    
begin
    Decoder: BP_Decoder
        generic map(
            NSTAGES => 4
        )
        port map(
            RESET => RESET,
            SYS_CLK => SYS_CLK,
            
            LOAD_KEY => LOAD_KEY,
            LOAD_PAIR => LOAD_PAIR,
            
            OUTPUT => OUTPUT,
            TAKE_OUTPUT => TAKE_OUTPUT,
            OUTPUT_VALID => OUTPUT_VALID,
            
            INPUT => INPUT,
            INPUT_CONSUMED => INPUT_CONSUMED
        );
    
    
    SYS_CLK_process: process
    begin
        SYS_CLK <= '0';
        wait for SYS_CLK_period/2;
        SYS_CLK <= '1';
        wait for SYS_CLK_period/2;
    end process;
    
    RESET_process: process
    begin
        RESET <= '1';
        wait for SYS_CLK_period;
        RESET <= '0';
        wait;
    end process;
    
    process(SYS_CLK)
        type pattern_type is record
            LOAD_KEY: std_logic;
            LOAD_PAIR: std_logic;
            DATA: std_logic_vector(7 downto 0);
        end record;
        type pattern_array is array (natural range <>) of pattern_type;
        
        constant test_patterns: pattern_array := (
            -- Pairs
            ('0', '1', x"A0"),
            ('0', '1', x"A1"),
            ('0', '1', x"A2"),
            ('0', '1', x"A3"),
            ('0', '1', x"A4"),
            ('0', '1', x"A5"),
            ('0', '1', x"A6"),
            ('0', '1', x"A7"),
            
            -- Keys
            ('1', '0', x"B0"),
            ('1', '0', x"B1"),
            ('1', '0', x"B2"),
            ('1', '0', x"B3"),
            
            -- Data
            ('0', '0', x"11"),
            ('0', '0', x"22"),
            ('0', '0', x"B0"),
            ('0', '0', x"33"),
            ('0', '0', x"44"),
            ('0', '0', x"B1"),
            ('0', '0', x"B2"),
            ('0', '0', x"B3"),
            ('0', '0', x"55"),
            ('0', '0', x"66"),
            ('0', '0', x"77"),
            ('0', '0', x"88"),
            
            -- Padding
            ('0', '0', x"00"),
            ('0', '0', x"00"),
            ('0', '0', x"00"),
            ('0', '0', x"00"),
            ('0', '0', x"00"),
            ('0', '0', x"00"),
            ('0', '0', x"00"),
            ('0', '0', x"00")
        );
        
        variable DATA_CTR: integer := 0;
    begin
        if(RESET = '0') then
            if(rising_edge(SYS_CLK)) then
                TAKE_OUTPUT <= '1';
--                 LOAD_KEY <= test_patterns(DATA_CTR).LOAD_KEY;
--                 LOAD_PAIR <= test_patterns(DATA_CTR).LOAD_PAIR;
--                 INPUT <= test_patterns(DATA_CTR).DATA;
                
                if(DATA_CTR = 0 or INPUT_CONSUMED = '1') then
                    if(DATA_CTR < test_patterns'length) then
                        LOAD_KEY <= test_patterns(DATA_CTR).LOAD_KEY;
                        LOAD_PAIR <= test_patterns(DATA_CTR).LOAD_PAIR;
                        INPUT <= test_patterns(DATA_CTR).DATA;
                        DATA_CTR := DATA_CTR + 1;
                    else
                        INPUT <= X"00";
                    end if;
                end if;
            end if;
        end if; -- RESET
    end process;
    
    
end Behavioral;

