
library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;

library UNISIM;
    use UNISIM.VComponents.all;

package BP_Decoder_pkg is
    component BP_DecodeStage is
        port(
            RESET:   in std_logic;-- asynchronous reset
            SYS_CLK: in std_logic;
            
            ENABLED:  in std_logic;
            IN_BYTE:  in std_logic_vector(7 downto 0);
            OUT_BYTE: out std_logic_vector(7 downto 0);
            
            READ:     in std_logic;
            EMPTY:    out std_logic;
            
            KEY:    in std_logic_vector(7 downto 0);
            FIRST:  in std_logic_vector(7 downto 0);
            SECOND: in std_logic_vector(7 downto 0)
        );
    end component;
    
    component BP_Decoder is
        generic(
            NSTAGES: integer
        );
        port(
            RESET:   in std_logic;-- asynchronous reset
            SYS_CLK: in std_logic;
            
            LOAD_KEY:  in  std_logic;
            LOAD_PAIR: in  std_logic;
            
            OUTPUT:      out std_logic_vector(7 downto 0);
            TAKE_OUTPUT: in  std_logic;
            OUTPUT_VALID: out std_logic;
            
            INPUT:          in  std_logic_vector(7 downto 0);
            INPUT_CONSUMED: out std_logic
        );
    end component;
end package BP_Decoder_pkg;

