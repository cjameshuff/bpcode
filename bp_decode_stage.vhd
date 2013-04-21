
library ieee;
    use ieee.std_logic_1164.all;
    use ieee.numeric_std.all;

library UNISIM;
    use UNISIM.VComponents.all;

entity BP_DecodeStage is
    port(
        RESET:   in std_logic;-- asynchronous reset
        SYS_CLK: in std_logic;
        
        ENABLED:  in std_logic;
        IN_BYTE:  in std_logic_vector(7 downto 0);
        OUT_BYTE: out std_logic_vector(7 downto 0);
        
        READ:   in std_logic;
        EMPTY:  out std_logic;
         
        KEY:    in std_logic_vector(7 downto 0);
        FIRST:  in std_logic_vector(7 downto 0);
        SECOND: in std_logic_vector(7 downto 0)
    );
end BP_DecodeStage;

architecture Behavioral of BP_DecodeStage is
    signal MATCHED: std_logic := '0';
    signal EMPTY_INTERN: std_logic := '0';
    
begin
    -- READ/EMPTY signal must be propagated ahead from later stages
    EMPTY <= READ and EMPTY_INTERN;
    
    MainProc: process(RESET, SYS_CLK) is
    begin
        if(RESET = '1') then
            MATCHED <= '0';
            EMPTY_INTERN <= '0';
            OUT_BYTE <= X"00";
        elsif(rising_edge(SYS_CLK)) then
            if(READ = '1') then
                EMPTY_INTERN <= '1';
                if(ENABLED = '1') then
                    if(MATCHED = '1') then
                        OUT_BYTE <= SECOND;
                        MATCHED <= '0';
                    else
                        if(IN_BYTE = KEY) then
                            OUT_BYTE <= FIRST;
                            MATCHED <= '1';
                            EMPTY_INTERN <= '0';
                        else
                            OUT_BYTE <= IN_BYTE;
                        end if;
                    end if;
                else
                    OUT_BYTE <= IN_BYTE;
                end if;
            end if; -- READ = '1'
        end if;
    end process;
end Behavioral;

