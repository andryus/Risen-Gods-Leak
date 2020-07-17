package parser;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.util.List;
import parser.ticket.Ticket;
import parser.ticket.TicketContainer;

/**
 * File Writer -> output sql to file
 * @author Pointerx
 */
public class SQLWriter {
    
    public SQLWriter()
    {
        List<Ticket> tickets = TicketContainer.getInstance().getTickets();
        tickets = TicketContainer.getInstance().filterTickets(tickets);
        File sqlFile = new File("rg_ticketstatus.sql");
        try {
           FileWriter writer = new FileWriter(sqlFile);
           writer.write("/* THIS WAS GENERATED AUTOMATICALLY */");
           breakLine(writer, 4);
           for (Ticket t : tickets)
           {
               writer.write("INSERT INTO rg_custom_tickets (ticketId, questId, priority, status) VALUES ("+t.getId()+", "+t.getQuestId()+", '"+t.getPriority()+"', '"+t.getStatus()+"');");
               breakLine(writer, 2);
               
           }
           writer.write("-- Successfully finished");
           writer.flush();
           writer.close();
        } catch (IOException e) {
            System.out.println(e.getLocalizedMessage());
        }
    }
    
    private void breakLine(FileWriter w, int count)
    {
        for (int i = 1; i < count; i++)
            try {
            w.write(System.getProperty("line.separator"));
        } catch (IOException ex) {
            System.out.println(ex.getLocalizedMessage());
            break;
        }
    }
}
