package parser.ticket;

import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import parser.xml.TicketParser;

/**
 * Globally used singleton to 
 * @author Pointerx
 */
public class TicketContainer {
    private List<Ticket> tickets;
    
    private TicketContainer() {
        tickets = TicketParser.getTickets();
    }
    
    public static TicketContainer getInstance() {
        return TicketContainerHolder.INSTANCE;
    }
    
    private static class TicketContainerHolder {

        private static final TicketContainer INSTANCE = new TicketContainer();
    }
    
    public List<Ticket> getTickets()
    {
        return tickets;
    }
    
    public List<Ticket> filterTickets(List<Ticket> t)
    {
        List<Ticket> ticks = new ArrayList<Ticket>();
        for (Ticket tick : t)
        {
            if (!tick.getPriority().equalsIgnoreCase("Niedrig") && !tick.getStatus().equalsIgnoreCase("Neu"))
            {
                Pattern intsOnly = Pattern.compile("(quest=|q=)\\d+");
                Matcher makeMatch = intsOnly.matcher(tick.getDescription());
                if (makeMatch.find())
                {
                    String entry = makeMatch.group();
                    tick.setQuestId(Integer.parseInt(entry.split("=")[1]));
                    ticks.add(tick);
                }
            }
        }
        return ticks;
    }
}
